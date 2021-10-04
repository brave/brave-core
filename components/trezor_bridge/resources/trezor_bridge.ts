// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { sendWithPromise } from 'chrome://resources/js/cr.m'
import apiProxy from './trezor_bridge_api_proxy.js'
const ethUtil = require('ethereumjs-util');
// Hooks to trick some checks inside TrezorConnect to use webextension env.
function setupBraveHooks () {
  // @ts-ignore
  window.sendWithPromise = sendWithPromise
}

setupBraveHooks()

import TrezorConnect, {
  UI,
  UI_EVENT
} from 'trezor-connect'

// Listen to UI_EVENT
// most common requests
TrezorConnect.on(UI_EVENT, (event) => {
  if (event.type === UI.REQUEST_PIN) {
      // example how to respond to pin request
      TrezorConnect.uiResponse({ type: UI.RECEIVE_PIN, payload: '1234' });
  }

  if (event.type === UI.REQUEST_PASSPHRASE) {
      const features = event.payload.device.features
      if (features && features.capabilities && features.capabilities.includes('Capability_PassphraseEntry')) {
          // device does support entering passphrase on device
          // let user choose where to enter
          // if he choose to do it on device respond with:
          TrezorConnect.uiResponse({
              type: UI.RECEIVE_PASSPHRASE,
              payload: { passphraseOnDevice: true, value: '', save: true },
          });
      } else {
          // example how to respond to passphrase request from regular UI input (form)
          TrezorConnect.uiResponse({
              type: UI.RECEIVE_PASSPHRASE,
              payload: { value: 'type your passphrase here', save: true },
          });
      }
  }

  if (event.type === UI.SELECT_DEVICE) {
      if (event.payload.devices.length > 0) {
          // more then one device connected
          // example how to respond to select device
          TrezorConnect.uiResponse({
              type: UI.RECEIVE_DEVICE,
              payload: { device: event.payload.devices[0], remember: true },
          });
      } else {
        console.log('no devices connected, waiting for connection')
      }
  }

  // getAddress from device which is not backed up
  // there is a high risk of coin loss at this point
  // warn user about it
  if (event.type === UI.REQUEST_CONFIRMATION) {
      // payload: true - user decides to continue anyway
      TrezorConnect.uiResponse({ type: UI.RECEIVE_CONFIRMATION, payload: true });
  }
})

const callbackRouter = apiProxy.getInstance().getCallbackRouter()
callbackRouter.requestAddresses.addListener(
  async (paths: string[]) => {
    if (!paths.length) {
      apiProxy.getInstance().onAddressesReceived(false, [], 'Error: paths empty')
      return
    }
    const requestedPaths = []
    for (const path of paths) {
      requestedPaths.push({path: path})
    }
    TrezorConnect.getPublicKey({ bundle: requestedPaths }).then(response => {
      const accounts = []
      const hardwareVendor = 'Trezor'
      for (let index = 0; index < (response.payload as any[]).length; index++) {
        const value = response.payload[index]
        const buffer = Buffer.from(value.publicKey, 'hex')
        const address = ethUtil.publicToAddress(buffer, true).toString('hex')
        accounts.push({
          address: ethUtil.toChecksumAddress(`0x${address}`),
          derivationPath: value.serializedPath,
          name: hardwareVendor + ' ' + value.serializedPath.substr(paths[0].lastIndexOf('/') + 1),
          hardwareVendor: hardwareVendor
        })
      }
      // @ts-ignore
      const error = response.payload.error ? response.payload.error : ''
      apiProxy.getInstance().onAddressesReceived(response.success, accounts, error)
    }).catch(error => {
      apiProxy.getInstance().onAddressesReceived(false, [], error.payload.error)
    })
  })

callbackRouter.unlock.addListener(async () => {
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
    env: 'web'
  }).then(() => {
    apiProxy.getInstance().onUnlocked(true, '');
  }).catch(error => {
    apiProxy.getInstance().onUnlocked(false, error);
  })
})
