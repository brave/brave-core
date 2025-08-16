// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {sendWithPromise} from 'chrome://resources/js/cr.js'

export type ContactInfo = {
  contactInfo: string | null
  contactInfoSaveFlag: boolean
}

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

  getAdBlockOnlyModeEnabled: () => Promise<boolean>
  setAdBlockOnlyModeEnabled: (value: boolean) => void

  getAdBlockOnlyModeSupported: () => Promise<boolean>

  getHttpsUpgradeControlType: () => Promise<string>
  setHttpsUpgradeControlType: (value: string) => void

  getNoScriptControlType: () => Promise<boolean>
  setNoScriptControlType: (value: boolean) => void

  getForgetFirstPartyStorageEnabled: () => Promise<boolean>
  setForgetFirstPartyStorageEnabled: (value: boolean) => void

  setContactInfoSaveFlag: (value: boolean) => void
  getContactInfo: () => Promise<ContactInfo>

  getHideBlockAllCookieTogle: () => Promise<boolean>

  getDeAmpEnabled: () => Promise<boolean>
  setDeAmpEnabled: (value: boolean) => void

  getDebounceEnabled: () => Promise<boolean>
  setDebounceEnabled: (value: boolean) => void

  getReduceLanguageEnabled: () => Promise<boolean>
  setReduceLanguageEnabled: (value: boolean) => void
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

  getAdBlockOnlyModeEnabled () {
    return sendWithPromise('getAdBlockOnlyModeEnabled')
  }

  setAdBlockOnlyModeEnabled (value: boolean) {
    chrome.send('setAdBlockOnlyModeEnabled', [value])
  }

  getAdBlockOnlyModeSupported () {
    return sendWithPromise('getAdBlockOnlyModeSupported')
  }

  getHttpsUpgradeControlType () {
    return sendWithPromise('getHttpsUpgradeControlType')
  }

  setHttpsUpgradeControlType (value: string) {
    chrome.send('setHttpsUpgradeControlType', [value])
  }

  getNoScriptControlType () {
    return sendWithPromise('getNoScriptControlType')
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

  getContactInfo () {
    return sendWithPromise('getContactInfo')
  }

  getHideBlockAllCookieTogle () {
    return sendWithPromise('getHideBlockAllCookieTogle')
  }

  getDeAmpEnabled () {
    return sendWithPromise('getDeAmpEnabled')
  }

  setDeAmpEnabled (value: boolean) {
    chrome.send('setDeAmpEnabled', [value])
  }

  getDebounceEnabled () {
    return sendWithPromise('getDebounceEnabled')
  }

  setDebounceEnabled (value: boolean) {
    chrome.send('setDebounceEnabled', [value])
  }

  getReduceLanguageEnabled () {
    return sendWithPromise('getReduceLanguageEnabled')
  }

  setReduceLanguageEnabled (value: boolean) {
    chrome.send('setReduceLanguageEnabled', [value])
  }

  static getInstance(): DefaultBraveShieldsBrowserProxy {
    return instance || (instance = new DefaultBraveShieldsBrowserProxyImpl())
  }
}

let instance: DefaultBraveShieldsBrowserProxy|null = null
