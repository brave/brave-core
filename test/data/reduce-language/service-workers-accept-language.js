addEventListener('install', function(event) {
  event.waitUntil(self.skipWaiting()); // Activate worker immediately
});

addEventListener('activate', function(event) {
  event.waitUntil(self.clients.claim()); // Become available to all pages
});

addEventListener('message', (event) => {
  if (event.data == 'fetch') {
    fetch('/reduce-language/empty.json')
      .then(r => r.text())
      .then(data => {
        event.source.postMessage('LOADED');
      })
      .catch((e) => {
        event.source.postMessage('FAILED');
      });
  }
});
