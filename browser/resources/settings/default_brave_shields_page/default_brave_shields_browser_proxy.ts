// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {sendWithPromise} from 'chrome://resources/js/cr.js'

export interface DefaultBraveShieldsBrowserProxy {
  /**
   * @return {!Promise<boolean>}
   */
  isAdControlEnabled(): Promise<boolean>
  /**
   * @param {string} value name.
   */
  setAdControlType(value: string): void

  /**
   * @return {!Promise<boolean>}
   */
  isFirstPartyCosmeticFilteringEnabled(): Promise<boolean>
  /**
   * @param {string} value name.
   */
  setCosmeticFilteringControlType(value: string): void

  /**
   * @return {!Promise<string>}
   */
  getCookieControlType(): Promise<string>
  /**
   * @param {string} value name.
   */
  setCookieControlType(value: string): void

  /**
   * @return {!Promise<string>}
   */
  getFingerprintingControlType(): Promise<string>
  /**
   * @param {string} value name.
   */
  setFingerprintingControlType(value: string): void

  /**
   * @return {!Promise<boolean>}
   */
  getFingerprintingBlockEnabled(): Promise<boolean>
  /**
   * @param {boolean} value name.
   */
  setFingerprintingBlockEnabled(value: boolean): void

  /**
   * @return {!Promise<string>}
   */
  getHttpsUpgradeControlType(): Promise<string>
  /**
   * @param {string} value name.
   */
  setHttpsUpgradeControlType(value: string): void

  /**
   * @param {string} value name.
   */
  setNoScriptControlType(value: string): void

  /**
   * @return {!Promise<boolean>}
   */
  getForgetFirstPartyStorageEnabled(): Promise<boolean>
  /**
   * @param {Boolean} value name.
   */
  setForgetFirstPartyStorageEnabled(value: boolean): void

  /**
   * @param {string} value name.
   */
  setContactInfoSaveFlag(value: boolean): void
  /**
   * @return {!Promise<boolean>}
   */
  getContactInfoSaveFlag(): Promise<boolean>
}

export class DefaultBraveShieldsBrowserProxyImpl implements DefaultBraveShieldsBrowserProxy {
  /** @override */
  isAdControlEnabled () {
    return sendWithPromise('isAdControlEnabled')
  }

  /** @override */
  setAdControlType (value: string) {
    chrome.send('setAdControlType', [value])
  }

  /** @override */
  isFirstPartyCosmeticFilteringEnabled () {
    return sendWithPromise('isFirstPartyCosmeticFilteringEnabled')
  }

  /** @override */
  setCosmeticFilteringControlType (value: string) {
    chrome.send('setCosmeticFilteringControlType', [value])
  }

  /** @override */
  getCookieControlType () {
    return sendWithPromise('getCookieControlType');
  }

  /** @override */
  setCookieControlType (value: string) {
    chrome.send('setCookieControlType', [value]);
  }

  /** @override */
  getFingerprintingControlType () {
    return sendWithPromise('getFingerprintingControlType');
  }

  /** @override */
  setFingerprintingControlType (value: string) {
    chrome.send('setFingerprintingControlType', [value])
  }

  /** @override */
  getFingerprintingBlockEnabled () {
    return sendWithPromise('getFingerprintingBlockEnabled')
  }

  /** @override */
  setFingerprintingBlockEnabled (value: boolean) {
    chrome.send('setFingerprintingBlockEnabled', [value])
  }

  /** @override */
  getHttpsUpgradeControlType () {
    return sendWithPromise('getHttpsUpgradeControlType')
  }

  /** @override */
  setHttpsUpgradeControlType (value: string) {
    chrome.send('setHttpsUpgradeControlType', [value])
  }

  /** @override */
  setNoScriptControlType (value: string) {
    chrome.send('setNoScriptControlType', [value])
  }

  /** @override */
  getForgetFirstPartyStorageEnabled () {
    return sendWithPromise('getForgetFirstPartyStorageEnabled')
  }

  /** @override */
  setForgetFirstPartyStorageEnabled (value: boolean) {
    chrome.send('setForgetFirstPartyStorageEnabled', [value])
  }

  /** @override */
  setContactInfoSaveFlag (value: boolean) {
    chrome.send('setContactInfoSaveFlag', [value])
  }

  /** @override */
  getContactInfoSaveFlag () {
    return sendWithPromise('getContactInfoSaveFlag')
  }

  static getInstance(): DefaultBraveShieldsBrowserProxy {
    return instance || (instance = new DefaultBraveShieldsBrowserProxyImpl())
  }


}

let instance: DefaultBraveShieldsBrowserProxy|null = null
