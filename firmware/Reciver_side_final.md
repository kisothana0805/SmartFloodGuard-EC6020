#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

// UART functions
void UART_init(unsigned int baud){
    unsigned int ubrr = F_CPU/16/baud-1;
    UBRR0H = (unsigned char)(ubrr>>8);
    UBRR0L = (unsigned char)ubrr;
    UCSR0B = (1<<TXEN0);
    UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);
}

void UART_send(char data){
    while(!(UCSR0A & (1<<UDRE0)));
    UDR0 = data;
}

void UART_print(char *str){
    while(*str) UART_send(*str++);
}

#define TANK_HEIGHT 100

#define TRIG PB1
#define ECHO PB2
#define RF_INPUT PD2
#define RESET_BTN PD3
#define DHT_DATA PD4
#define RELAY PD5
#define BUZZER PD6
#define WATER_OUT PB0   // Example: Arduino D8

uint8_t system_active = 0;

uint16_t buzzer_timer = 0;
uint16_t relay_timer = 0;
uint8_t rf_confirm_count = 0;


// ================= I2C =================

void I2C_init(){
    TWSR = 0x00;
    TWBR = 72;
    TWCR = (1<<TWEN);
}

void I2C_start(uint8_t address){
    TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
    while(!(TWCR & (1<<TWINT)));

    TWDR = address;
    TWCR = (1<<TWINT)|(1<<TWEN);
    while(!(TWCR & (1<<TWINT)));
}

void I2C_write(uint8_t data){
    TWDR = data;
    TWCR = (1<<TWINT)|(1<<TWEN);
    while(!(TWCR & (1<<TWINT)));
}

void I2C_stop(){
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
}

// ================= LCD =================

#define LCD_ADDR (0x3F<<1)
#define EN 0x04
#define RS 0x01

void lcd_send(uint8_t data,uint8_t mode){

    uint8_t high = data & 0xF0;
    uint8_t low = (data<<4) & 0xF0;

    I2C_start(LCD_ADDR);

    I2C_write(high | mode | EN | 0x08);
    I2C_write(high | mode | 0x08);

    I2C_write(low | mode | EN | 0x08);
    I2C_write(low | mode | 0x08);

    I2C_stop();
}

void lcd_cmd(uint8_t cmd){
    lcd_send(cmd,0);
    _delay_ms(2);
}

void lcd_data(uint8_t data){
    lcd_send(data,RS);
}

void lcd_init(){

    _delay_ms(50);

    lcd_cmd(0x33);
    lcd_cmd(0x32);
    lcd_cmd(0x28);
    lcd_cmd(0x0C);
    lcd_cmd(0x06);
    lcd_cmd(0x01);
}

void lcd_set_cursor(uint8_t r,uint8_t c){

    uint8_t pos;

    if(r==0)
        pos = 0x80 + c;
    else
        pos = 0xC0 + c;

    lcd_cmd(pos);
}

void lcd_print(char *str){
    while(*str) lcd_data(*str++);
}

// ================= BUZZER =================

void buzzer_pwm_init(){

    DDRD |= (1<<BUZZER);

    TCCR0A |= (1<<COM0A1);
    TCCR0A |= (1<<WGM00)|(1<<WGM01);

    TCCR0B |= (1<<CS01);
}

void buzzer_volume(uint8_t level){
    OCR0A = level;
}

// ================= ULTRASONIC =================

void ultrasonic_trigger(){

    PORTB &= ~(1<<TRIG);
    _delay_us(2);

    PORTB |= (1<<TRIG);
    _delay_us(10);

    PORTB &= ~(1<<TRIG);
}

uint16_t measure_echo_timer1(){

    TCNT1 = 0;

    while(!(PINB & (1<<ECHO)));

    TCCR1B |= (1<<CS11);

    while(PINB & (1<<ECHO));

    TCCR1B &= ~(1<<CS11);

    return TCNT1;
}

// ================= DHT11 =================

uint8_t DHT_read_byte()
{
    uint8_t i, data=0;

    for(i=0;i<8;i++)
    {
        while(!(PIND & (1<<DHT_DATA)));
        _delay_us(30);

        if(PIND & (1<<DHT_DATA))
            data |= (1<<(7-i));

        while(PIND & (1<<DHT_DATA));
    }

    return data;
}

uint8_t DHT_getHumidity()
{
    uint8_t humidity_int, humidity_dec, temp_int, temp_dec, checksum;

    DDRD |= (1<<DHT_DATA);
    PORTD &= ~(1<<DHT_DATA);
    _delay_ms(20);

    PORTD |= (1<<DHT_DATA);
    _delay_us(30);

    DDRD &= ~(1<<DHT_DATA);

    if(PIND & (1<<DHT_DATA)) return 0;

    while(!(PIND & (1<<DHT_DATA)));
    while(PIND & (1<<DHT_DATA));

    humidity_int = DHT_read_byte();
    humidity_dec = DHT_read_byte();
    temp_int = DHT_read_byte();
    temp_dec = DHT_read_byte();
    checksum = DHT_read_byte();

    return humidity_int;
}

uint8_t rf_detected(void)
{
    uint8_t activity = 0;

    for (uint8_t i = 0; i < 30; i++)
    {
        if (PIND & (1 << RF_INPUT))
            activity++;

        _delay_ms(1);
    }

    return (activity >= 10);
}

// ================= MAIN =================

int main(void)
{

    DDRB |= (1<<TRIG);
    DDRB &= ~(1<<ECHO);
    
    DDRB |= (1<<WATER_OUT);   // set as output
    PORTB &= ~(1<<WATER_OUT); // initially LOW

    DDRD &= ~(1<<RF_INPUT);
    
    DDRD &= ~(1<<RESET_BTN);

    DDRD |= (1<<RELAY);

    PORTD |= (1<<RESET_BTN);
    PORTD |= (1<<RELAY);
    PORTD &= ~(1<<RF_INPUT); 

    I2C_init();
    lcd_init();
    UART_init(9600);  // Add after lcd_init() and buzzer_pwm_init()
    buzzer_pwm_init();

    char buffer[16];

    while(1)
    {
    // RF trigger (stable detection)
if (!system_active)
{
    if (rf_detected())
    {
        rf_confirm_count++;
        if (rf_confirm_count >= 2)
        {
            system_active = 1;
            rf_confirm_count = 0;
        }
    }
    else
    {
        rf_confirm_count = 0;
    }
}

        // reset button
        if(!(PIND & (1<<RESET_BTN)))
        {
            rf_confirm_count = 0;
            system_active = 0;
            buzzer_timer = 0;
            buzzer_volume(0);
            relay_timer = 0;
            PORTD |= (1<<RELAY);
            PORTB &= ~(1<<WATER_OUT);
            lcd_cmd(0x01);
        }

        if(system_active)
        {
            PORTB |= (1<<WATER_OUT);   // OUTPUT HIGH
            ultrasonic_trigger();

            uint16_t counts = measure_echo_timer1();
            uint16_t distance = counts / 116;

            uint16_t depth;

            if(distance >= TANK_HEIGHT)
                depth = 0;
            else
                depth = TANK_HEIGHT - distance;

            uint8_t humidity = DHT_getHumidity();

            lcd_set_cursor(0,0);
            sprintf(buffer,"Depth:%ucm ",depth);
            lcd_print(buffer);

            lcd_set_cursor(1,0);
            sprintf(buffer,"Hum:%u%% ",humidity);
            lcd_print(buffer);
            
            uint8_t relay_state = (PORTD & (1<<RELAY)) ? 0 : 1;

            char tx_buffer[50];
            sprintf(tx_buffer,"D:%u,H:%u,W:1,R:%u\n",depth,humidity,relay_state);
            UART_print(tx_buffer);

            // FAST BUZZER BEEP
            buzzer_timer++;

            if(buzzer_timer < 3)
                buzzer_volume(255);
            else if(buzzer_timer < 6)
                buzzer_volume(0);
            else
                buzzer_timer = 0;

            // relay timer
            relay_timer++;

            if(relay_timer > 300)
            {
                PORTD &= ~(1<<RELAY);
            }

        }
        else
        {
            PORTB &= ~(1<<WATER_OUT);  // OUTPUT LOW
            buzzer_timer = 0;
            buzzer_volume(0);
            
            lcd_set_cursor(0,0);
            lcd_print("Water Not");
            lcd_set_cursor(1,0);
            lcd_print("Detected ");
            
            char tx_buffer[50];
            sprintf(tx_buffer,"D:0,H:0,W:0,R:0\n");
            UART_print(tx_buffer);
        }

        _delay_ms(50);
    }
}
