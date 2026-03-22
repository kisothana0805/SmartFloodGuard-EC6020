#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

void send_rf_command(void)
{
    for (uint8_t p = 0; p < 5; p++)   // repeat packet
    {
        for (uint8_t i = 0; i < 30; i++)
        {
            PORTD |= (1 << PD4);
            _delay_ms(1);
            PORTD &= ~(1 << PD4);
            _delay_ms(1);
        }
        _delay_ms(20);
    }
}

int main(void)
{
    DDRD &= ~(1 << PD2);   // Water sensor input (D2)
    DDRD |=  (1 << PD4);   // RF TX DATA (D4)
    DDRB |=  (1 << PB5);   // TX LED (D13)

    PORTB &= ~(1 << PB5);  // LED OFF at power ON

    while (1)
    {
        if (PIND & (1 << PD2))   // Water detected
        {
            PORTB |= (1 << PB5);    // TX LED ON
            send_rf_command();      // Send RF
            _delay_ms(500);         // avoid repeats
        }
        else
        {
            PORTB &= ~(1 << PB5);   // TX LED OFF
            PORTD &= ~(1 << PD4);   // RF idle
        }
    }
} 