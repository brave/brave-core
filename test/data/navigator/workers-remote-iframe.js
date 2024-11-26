/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

 window.onmessage = function (event) {
  if (navigator.userAgent == event.data) {
    document.title = 'pass'
  }
}

var iframe = document.createElement('iframe')
iframe.src = 'https://z.com/navigator/workers-iframe.html'
document.body.appendChild(iframe)
