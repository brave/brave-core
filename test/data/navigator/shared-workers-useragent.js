/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

worker = new SharedWorker('shared-workers-worker.js')
worker.port.onmessage = function (e) {
  if (navigator.userAgent == e.data) {
    document.title = 'pass'
  } else {
    document.title = 'fail'
  }
}
worker.port.start()
