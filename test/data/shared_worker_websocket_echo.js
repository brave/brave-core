/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Opens a WebSocket to the URL posted on the port and relays the first message
// received from the server back to the page.
onconnect = event => {
  const port = event.ports[0];
  port.onmessage = event => {
    const socket = new WebSocket(event.data);
    socket.onmessage = message => {
      port.postMessage(message.data);
      socket.close();
    };
    socket.onerror = () => port.postMessage('WebSocket error');
  };
};
