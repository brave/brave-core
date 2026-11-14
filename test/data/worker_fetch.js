/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Fetches the URL it is sent and replies with whether the request succeeded.
// Used as a dedicated, shared and service worker.

async function checkFetch(url) {
  try {
    await fetch(url, { mode: 'no-cors' });
    return true;
  } catch {
    return false;
  }
}

// Dedicated and service workers
self.onmessage = async event => {
  (event.source ?? self).postMessage(await checkFetch(event.data));
};

// Shared workers
self.onconnect = event => {
  const port = event.ports[0];
  port.onmessage = async message => {
    port.postMessage(await checkFetch(message.data));
  };
};
