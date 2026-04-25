self.addEventListener('install', function(event) {
  event.waitUntil(self.skipWaiting()); // Activate worker immediately
});

self.addEventListener('activate', function(event) {
  event.waitUntil(self.clients.claim()); // Become available to all pages
});
self.addEventListener('message', async event => {
  const client = await clients.get(event.source.id)
  client.postMessage(navigator.userAgent);
});
