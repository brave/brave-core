onconnect = event => {
  const port = event.ports[0];
  port.onmessage = event => {
    const socket = new WebSocket(event.data);
    let completed = false;
    const report = connected => {
      if (completed) {
        return;
      }
      completed = true;
      port.postMessage(connected);
    };
    socket.onopen = () => report(true);
    socket.onerror = () => report(false);
  };
};
