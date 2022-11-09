// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import { sendWithPromise } from 'chrome://resources/js/cr.m.js';

export interface BraveTorBrowserProxy {
  getBridgesConfig(): Promise<any> // TODO(petemill): Define the expected type
  setBridgesConfig(config: any) // TODO(petemill): Define the expected type
  requestBridgesCaptcha(): Promise<any> // TODO(petemill): Define the expected type
  resolveBridgesCaptcha(captcha: any) // TODO(petemill): Define the expected type
  setTorEnabled(value: boolean)
  isTorEnabled(): Promise<boolean>
  isTorManaged(): Promise<boolean>
}

export class BraveTorBrowserProxyImpl implements BraveTorBrowserProxy {
  static getInstance() {
    return instance || (instance = new BraveTorBrowserProxyImpl());
  }

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

let instance: BraveTorBrowserProxy|null = null
