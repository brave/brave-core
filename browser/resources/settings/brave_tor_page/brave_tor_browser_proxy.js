// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { sendWithPromise } from 'chrome://resources/js/cr.m.js';

/** @interface */
export class BraveTorBrowserProxy {
  getInstance() { }

  getBridgesConfig() { }
  setBridgesConfig() { }
  requestBridgesCaptcha() { }
  resolveBridgesCaptcha() { }

  setTorEnabled() { }
  isTorEnabled() { }
  isTorManaged() { }
}

/**
* @implements {BraveTorBrowserProxy}
*/
export class BraveTorBrowserProxyImpl {
  /** @instance */
  static getInstance() {
    return instance || (instance = new BraveTorBrowserProxyImpl());
  }

  /** @returns {Promise} */
  getBridgesConfig() {
    return sendWithPromise('brave_tor.getBridgesConfig')
  }

  setBridgesConfig(config) {
    chrome.send('brave_tor.setBridgesConfig', [config])
  }

  requestBridgesCaptcha() {
    return sendWithPromise('brave_tor.requestBridgesCaptcha')
  }

  resolveBridgesCaptcha(captcha) {
    return sendWithPromise('brave_tor.resolveBridgesCaptcha', captcha)
  }

  setTorEnabled(value) {
    chrome.send('brave_tor.setTorEnabled', [value])
  }

  isTorEnabled() {
    return sendWithPromise('brave_tor.isTorEnabled')
  }

  isTorManaged() {
    return sendWithPromise('brave_tor.isTorManaged')
  }
}

/** @type {BraveTorBrowserProxyImpl} */
let instance