/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

addEventListener('install', function (event) {
  event.waitUntil(self.skipWaiting()); // Activate worker immediately
});

addEventListener('activate', function (event) {
  event.waitUntil(self.clients.claim()); // Become available to all pages
});

sources = [];

addEventListener('message', async event => {
  if (event.data.cmd === 'open_es') {
    const client = await self.clients.get(event.source.id);
    source = new EventSource(event.data.url);
    sources.push(source);
    let openEventListener, errorEventListener;
    const removeEventListeners = () => {
      source.removeEventListener("open", openEventListener);
      source.removeEventListener("error", errorEventListener);
    };
    openEventListener = (event) => {
      client.postMessage('open');
      removeEventListeners();
    };
    errorEventListener = (event) => {
      client.postMessage('error');
      removeEventListeners();
    };
    source.addEventListener('open', openEventListener);
    source.addEventListener('error', errorEventListener);
  } else if (event.data.cmd === 'close_es') {
    sources[event.data.idx].close();
  }
});
