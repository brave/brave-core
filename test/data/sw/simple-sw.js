/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
self.addEventListener('fetch', (event) => {
  const url = new URL(event.request.url);
  if (!url.pathname.endsWith('loading.html')) {
    return;  // use default fetch
  }
  console.log('SW intercepted a request for ' + event.request.url);
  console.log('SW sees searchParams as ' + url.searchParams.toString());
  if (url.searchParams.get('fbclid')) {
    console.log('SW found the tracking param');
    event.respondWith(fetch('/sw/tracked.html'));
  } else {
    console.log('SW couldn\'t find the tracking param');
    event.respondWith(fetch('/sw/not-tracked.html'));
  }
});
