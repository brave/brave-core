// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

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
