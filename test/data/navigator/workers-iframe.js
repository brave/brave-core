/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

 console.log(document.location.href)

var worker = function () {
  postMessage(navigator.userAgent)
}

var workerBlob = new Blob(['(' + worker.toString() + ')()'], {
  type: 'text/javascript'
})

worker = new Worker(window.URL.createObjectURL(workerBlob))
worker.onmessage = function (e) {
  window.parent.postMessage(e.data, '*')
}
