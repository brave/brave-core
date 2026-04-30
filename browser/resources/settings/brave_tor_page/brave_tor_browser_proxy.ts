// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { sendWithPromise } from 'chrome://resources/js/cr.js';

// TODO(petemill): Define the expected types instead of using any
export interface BraveTorBrowserProxy {
  getBridgesConfig: () => Promise<any>
  setBridgesConfig: (config: any) => void
  requestBridgesCaptcha: () => Promise<any>
  resolveBridgesCaptcha: (captcha: any) => Promise<any>
  setTorEnabled: (value: boolean) => void
  isTorEnabled: () => Promise<boolean>
  isTorManaged: () => Promise<boolean>
  isSnowflakeExtensionAllowed: () => Promise<boolean>
  isSnowflakeExtensionEnabled: () => Promise<boolean>
  enableSnowflakeExtension: (enable: boolean) => Promise<boolean>
}

export class BraveTorBrowserProxyImpl implements BraveTorBrowserProxy {
  static getInstance() {
    return instance || (instance = new BraveTorBrowserProxyImpl());
  }

  getBridgesConfig() {
    return sendWithPromise<any>('brave_tor.getBridgesConfig')
  }

  setBridgesConfig(config: any) {
    chrome.send('brave_tor.setBridgesConfig', [config])
  }

  requestBridgesCaptcha() {
    return sendWithPromise<any>('brave_tor.requestBridgesCaptcha')
  }

  resolveBridgesCaptcha(captcha: any) {
    return sendWithPromise<any>('brave_tor.resolveBridgesCaptcha', captcha)
  }

  setTorEnabled(value: boolean) {
    chrome.send('brave_tor.setTorEnabled', [value])
  }

  isTorEnabled() {
    return sendWithPromise<boolean>('brave_tor.isTorEnabled')
  }

  isTorManaged() {
    return sendWithPromise<boolean>('brave_tor.isTorManaged')
  }

  isSnowflakeExtensionAllowed() {
    return sendWithPromise<boolean>('brave_tor.isSnowflakeExtensionAllowed')
  }

  isSnowflakeExtensionEnabled() {
    return sendWithPromise<boolean>('brave_tor.isSnowflakeExtensionEnabled')
  }

  enableSnowflakeExtension(enable: boolean) {
    return sendWithPromise<boolean>('brave_tor.enableSnowflakeExtension', enable)
  }
}

let instance: BraveTorBrowserProxy|null = null
