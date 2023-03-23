/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Register service worker assuming it will claim all clients.
async function registerServiceWorker(script, scope = './') {
  await navigator.serviceWorker.register(script, { scope: scope })
  await navigator.serviceWorker.ready
  if (navigator.serviceWorker.controller) {
    return true
  }
  return await new Promise((resolve, reject) => {
    navigator.serviceWorker.oncontrollerchange = () => {
      if (navigator.serviceWorker.controller) {
        resolve(true)
      } else {
        reject('navigator.serviceWorker.controller is not set')
      }
    }
  })
}

// Sends a message to the service worker and returns a response.
async function messageServiceWorker(...args) {
  var channel = new MessageChannel()
  var responsePromise = new Promise((resolve, reject) => {
    channel.port1.onmessage = resolve
    channel.port1.onmessageerror = reject
  })
  navigator.serviceWorker.controller.postMessage(args, [channel.port2])
  const responseData = (await responsePromise).data
  if (responseData.error) {
    throw responseData.error
  }
  return responseData.data
}
