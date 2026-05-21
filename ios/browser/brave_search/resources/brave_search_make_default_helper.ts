// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {sendWebKitMessageWithReply} from
    '//ios/web/public/js_messaging/resources/utils.js';

const allowedOrigins = [
  'https://safesearch.brave.com',
  'https://safesearch.brave.software',
  'https://safesearch.bravesoftware.com',
  'https://search-dev-local.brave.com',
  'https://search.brave.com',
  'https://search.brave.software',
  'https://search.bravesoftware.com',
];

if (allowedOrigins.includes(window.location.origin)) {
  Object.defineProperty(window, 'brave', {
    enumerable: false,
    configurable: false,
    writable: false,
    value: {
      getCanSetDefaultSearchProvider() {
        return sendWebKitMessageWithReply(
            'BraveSearchMakeDefaultMessageHandler', {method_id: 1});
      },

      setIsDefaultSearchProvider() {
        return sendWebKitMessageWithReply(
            'BraveSearchMakeDefaultMessageHandler', {method_id: 2});
      }
    }
  });
}
