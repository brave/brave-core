// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

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

  isSnowflakeExtensionAllowed() {
    return sendWithPromise('brave_tor.isSnowflakeExtensionAllowed')
  }

  isSnowflakeExtensionEnabled(): Promise<boolean> {
    return sendWithPromise('brave_tor.isSnowflakeExtensionEnabled')
  }

  enableSnowflakeExtension(enable): Promise<boolean> {
    return sendWithPromise('brave_tor.enableSnowflakeExtension', enable)
  }
}

let instance: BraveTorBrowserProxy|null = null
