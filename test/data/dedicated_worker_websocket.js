/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

onmessage = event => {
  const socket = new WebSocket(event.data);
  let completed = false;
  const report = connected => {
    if (completed) {
      return;
    }
    completed = true;
    postMessage(connected);
  };
  socket.onopen = () => report(true);
  socket.onerror = () => report(false);
};
