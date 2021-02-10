self.addEventListener('message', (event) => {
  if (event.data == 'fetch') {
    fetch('/adbanner.js')
      .then(r => r.text())
      .then(data => {
        event.source.postMessage('LOADED');
      })
      .catch((e) => {
        event.source.postMessage('FAILED');
      });
  }
});
