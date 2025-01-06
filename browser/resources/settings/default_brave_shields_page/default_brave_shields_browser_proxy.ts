// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {sendWithPromise} from 'chrome://resources/js/cr.js'

export interface DefaultBraveShieldsBrowserProxy {
  isAdControlEnabled: () => Promise<boolean>
  setAdControlType: (value: boolean) => void

  isFirstPartyCosmeticFilteringEnabled: () => Promise<boolean>
  setCosmeticFilteringControlType: (value: string) => void

  getCookieControlType: () => Promise<string>
  setCookieControlType: (value: string) => void

  getFingerprintingControlType: () => Promise<string>
  setFingerprintingControlType: (value: string) => void

  getFingerprintingBlockEnabled: () => Promise<boolean>
  setFingerprintingBlockEnabled: (value: boolean) => void

  getHttpsUpgradeControlType: () => Promise<string>
  setHttpsUpgradeControlType: (value: string) => void

  setNoScriptControlType: (value: boolean) => void

  getForgetFirstPartyStorageEnabled: () => Promise<boolean>
  setForgetFirstPartyStorageEnabled: (value: boolean) => void

  setContactInfoSaveFlag: (value: boolean) => void
  getContactInfoSaveFlag: () => Promise<boolean>

  getHideBlockAllCookieTogle: () => Promise<boolean>
}

export class DefaultBraveShieldsBrowserProxyImpl
implements DefaultBraveShieldsBrowserProxy {
  isAdControlEnabled () {
    return sendWithPromise('isAdControlEnabled')
  }

  setAdControlType (value: boolean) {
    chrome.send('setAdControlType', [value])
  }

  isFirstPartyCosmeticFilteringEnabled () {
    return sendWithPromise('isFirstPartyCosmeticFilteringEnabled')
  }

  setCosmeticFilteringControlType (value: string) {
    chrome.send('setCosmeticFilteringControlType', [value])
  }

  getCookieControlType () {
    return sendWithPromise('getCookieControlType')
  }

  setCookieControlType (value: string) {
    chrome.send('setCookieControlType', [value])
  }

  getFingerprintingControlType () {
    return sendWithPromise('getFingerprintingControlType')
  }

  setFingerprintingControlType (value: string) {
    chrome.send('setFingerprintingControlType', [value])
  }

  getFingerprintingBlockEnabled () {
    return sendWithPromise('getFingerprintingBlockEnabled')
  }

  setFingerprintingBlockEnabled (value: boolean) {
    chrome.send('setFingerprintingBlockEnabled', [value])
  }

  getHttpsUpgradeControlType () {
    return sendWithPromise('getHttpsUpgradeControlType')
  }

  setHttpsUpgradeControlType (value: string) {
    chrome.send('setHttpsUpgradeControlType', [value])
  }

  setNoScriptControlType (value: boolean) {
    chrome.send('setNoScriptControlType', [value])
  }

  getForgetFirstPartyStorageEnabled () {
    return sendWithPromise('getForgetFirstPartyStorageEnabled')
  }

  setForgetFirstPartyStorageEnabled (value: boolean) {
    chrome.send('setForgetFirstPartyStorageEnabled', [value])
  }

  setContactInfoSaveFlag (value: boolean) {
    chrome.send('setContactInfoSaveFlag', [value])
  }

  getContactInfoSaveFlag () {
    return sendWithPromise('getContactInfoSaveFlag')
  }

  getHideBlockAllCookieTogle () {
    return sendWithPromise('getHideBlockAllCookieTogle')
  }

  static getInstance(): DefaultBraveShieldsBrowserProxy {
    return instance || (instance = new DefaultBraveShieldsBrowserProxyImpl())
  }
}

let instance: DefaultBraveShieldsBrowserProxy|null = null
