addEventListener('install', function(event) {
  event.waitUntil(self.skipWaiting()); // Activate worker immediately
});

addEventListener('activate', function(event) {
  event.waitUntil(self.clients.claim()); // Become available to all pages
});

sockets = []

addEventListener('message', async event => {
  if (event.data.cmd === 'open_ws') {
    const client = await clients.get(event.source.id)
    socket = new WebSocket(event.data.url);
    sockets.push(socket);
    socket.addEventListener('open', () => client.postMessage('open'));
    socket.addEventListener('error', () => client.postMessage('error'));
  } else if (event.data.cmd === 'close_ws') {
    sockets[event.data.idx].close();
  }
});
