// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { sendWithPromise } from 'chrome://resources/js/cr.m'

// Hooks to trick some checks inside TrezorConnect to use webextension env.
function setupBraveHooks () {
  if (!window.chrome) {
    window.chrome = Object({})
  }
  if (!window.chrome.runtime) {
    window.chrome.runtime = Object({})
  }
  if (!window.chrome.runtime.id) {
    window.chrome.runtime.id = ''
  }
  if (!window.chrome.runtime.onConnect) {
    window.chrome.runtime.onConnect = Object({
      addListener: () => undefined
    })
  }
  // @ts-ignore
  window.sendWithPromise = sendWithPromise
}

setupBraveHooks()

import TrezorConnect, {
  TRANSPORT_EVENT,
  DEVICE_EVENT,
  TRANSPORT
} from 'trezor-connect'

// Listen to TRANSPORT_EVENT
TrezorConnect.on(TRANSPORT_EVENT, event => {
  if (event.type === TRANSPORT.ERROR) {
    console.log('Transport is missing')
  }
  if (event.type === TRANSPORT.START) {
    console.log(event)
  }
})

// Listen to DEVICE_EVENT
TrezorConnect.on(DEVICE_EVENT, event => {
  console.log(event.type)
})

TrezorConnect.init({
  connectSrc: './trezor/',
  popup: false, // render your own UI
  webusb: false, // webusb is not supported
  debug: false, // see what's going on inside connect
  // lazyLoad: true, // set to "false" (default) if you want to start communication with bridge on application start (and detect connected device right away)
  // set it to "true", then trezor-connect will not be initialized until you call some TrezorConnect.method()
  // this is useful when you don't know if you are dealing with Trezor user
  manifest: {
    email: 'support@brave.com',
    appUrl: 'web-ui-boilerplate'
  },
  env: 'webextension'
}).then(() => {
  console.log('TrezorConnect is ready!')
  TrezorConnect.getPublicKey({
    path: "m/49'/0'/0'",
    coin: 'btc'
  }).then(response => {
    console.log(response)
  })
}).catch(error => {
  console.log('TrezorConnect init error', `TrezorConnect init error:${error}`)
})
