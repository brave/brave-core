// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { sendWithPromise } from 'chrome://resources/js/cr.js';

export interface BridgesConfig {
  use_bridges: number
  use_builtin_bridges: number
  requested_bridges: string[]
  provided_bridges: string[]
}

export interface BridgesCaptcha {
  captcha: string
}

export interface BridgesCaptchaResponse {
  bridges: Object[]
}

export interface BraveTorBrowserProxy {
  getBridgesConfig: () => Promise<BridgesConfig>
  setBridgesConfig: (config: BridgesConfig) => void
  requestBridgesCaptcha: () => Promise<BridgesCaptcha>
  resolveBridgesCaptcha: (captcha: string) => Promise<BridgesCaptchaResponse>
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
    return sendWithPromise<BridgesConfig>('brave_tor.getBridgesConfig')
  }

  setBridgesConfig(config: BridgesConfig) {
    chrome.send('brave_tor.setBridgesConfig', [config])
  }

  requestBridgesCaptcha() {
    return sendWithPromise<BridgesCaptcha>('brave_tor.requestBridgesCaptcha')
  }

  resolveBridgesCaptcha(captcha: string) {
    return sendWithPromise<BridgesCaptchaResponse>(
      'brave_tor.resolveBridgesCaptcha', captcha)
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
