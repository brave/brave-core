// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {sendWebKitMessage} from
    '//ios/web/public/js_messaging/resources/utils.js';

const allowedOrigins = [
  'https://talk.brave.com',
  'https://talk.bravesoftware.com',
  'https://talk.brave.software',
];

if (allowedOrigins.includes(window.location.origin)) {
  const observer = new MutationObserver((mutations: MutationRecord[]) => {
    for (const mutation of mutations) {
      for (const node of mutation.addedNodes) {
        if (node instanceof HTMLIFrameElement) {
          sendWebKitMessage(
              'BraveTalkLauncherMessageHandler', {url: node.src});
          observer.disconnect();
          return;
        }
      }
    }
  });

  observer.observe(document, {childList: true, subtree: true});
}
