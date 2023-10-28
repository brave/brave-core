// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import {sendWithPromise} from 'chrome://resources/js/cr.js';

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

export interface DefaultBraveShieldsBrowserProxy {
  /**
   * @return {!Promise<boolean>}
   */
  isAdControlEnabled()
  /**
   * @param {string} value name.
   */
  setAdControlType(value)

  /**
   * @return {!Promise<boolean>}
   */
  isFirstPartyCosmeticFilteringEnabled()
  /**
   * @param {string} value name.
   */
  setCosmeticFilteringControlType(value)

  /**
   * @return {!Promise<string>}
   */
  getCookieControlType()
  /**
   * @param {string} value name.
   */
  setCookieControlType(value)

  /**
   * @return {!Promise<string>}
   */
  getFingerprintingControlType()
  /**
   * @param {string} value name.
   */
  setFingerprintingControlType(value)

  /**
   * @return {!Promise<string>}
   */
  getHttpsUpgradeControlType()
  /**
   * @param {string} value name.
   */
  setHttpsUpgradeControlType(value)

  /**
   * @param {string} value name.
   */
  setNoScriptControlType(value)

  /**
   * @return {!Promise<Boolean>}
   */
  getForgetFirstPartyStorageEnabled()
  /**
   * @param {Boolean} value name.
   */
  setForgetFirstPartyStorageEnabled(value)
}

export class DefaultBraveShieldsBrowserProxyImpl implements DefaultBraveShieldsBrowserProxy {
  /** @override */
  isAdControlEnabled() {
    return sendWithPromise('isAdControlEnabled');
  }

  /** @override */
  setAdControlType(value) {
    chrome.send('setAdControlType', [value]);
  }

  /** @override */
  isFirstPartyCosmeticFilteringEnabled() {
    return sendWithPromise('isFirstPartyCosmeticFilteringEnabled');
  }

  /** @override */
  setCosmeticFilteringControlType(value) {
    chrome.send('setCosmeticFilteringControlType', [value]);
  }

  /** @override */
  getCookieControlType() {
    return sendWithPromise('getCookieControlType');
  }

  /** @override */
  setCookieControlType(value) {
    chrome.send('setCookieControlType', [value]);
  }

  /** @override */
  getFingerprintingControlType() {
    return sendWithPromise('getFingerprintingControlType');
  }

  /** @override */
  setFingerprintingControlType(value) {
    chrome.send('setFingerprintingControlType', [value]);
  }

  /** @override */
  getHttpsUpgradeControlType() {
    return sendWithPromise('getHttpsUpgradeControlType');
  }

  /** @override */
  setHttpsUpgradeControlType(value) {
    chrome.send('setHttpsUpgradeControlType', [value]);
  }

  /** @override */
  setNoScriptControlType(value) {
    chrome.send('setNoScriptControlType', [value]);
  }

  /** @override */
  getForgetFirstPartyStorageEnabled() {
    return sendWithPromise('getForgetFirstPartyStorageEnabled');
  }

  /** @override */
  setForgetFirstPartyStorageEnabled(value) {
    chrome.send('setForgetFirstPartyStorageEnabled', [value]);
  }

  static getInstance(): DefaultBraveShieldsBrowserProxy {
    return instance || (instance = new DefaultBraveShieldsBrowserProxyImpl())
  }
}

let instance: DefaultBraveShieldsBrowserProxy|null = null
