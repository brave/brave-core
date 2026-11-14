/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Posts the `command` query parameter as an education page's browser command.
window.onload = () => {
  window.top!.postMessage(
    {
      messageType: 'browser-command',
      command: new URLSearchParams(window.location.search).get('command'),
    },
    'chrome://getting-started/',
  )
}
