const CACHE_NAME = "flood-monitor-v1";

const ASSETS = [
  "/flood_monitor/app/",
  "/flood_monitor/app/index.html",
  "/flood_monitor/app/offline.html",
  "/flood_monitor/app/manifest.json"
];

// Install: cache basic shell
self.addEventListener("install", (event) => {
  event.waitUntil(
    caches.open(CACHE_NAME).then((cache) => cache.addAll(ASSETS))
  );
});

// Activate: cleanup old caches
self.addEventListener("activate", (event) => {
  event.waitUntil(
    caches.keys().then((keys) =>
      Promise.all(keys.map((k) => (k !== CACHE_NAME ? caches.delete(k) : null)))
    )
  );
});

// Fetch: network-first for app pages, fallback to offline
self.addEventListener("fetch", (event) => {
  const req = event.request;

  event.respondWith(
    fetch(req)
      .then((res) => res)
      .catch(() => caches.match(req).then((cached) => cached || caches.match("/flood_monitor/app/offline.html")))
  );
});