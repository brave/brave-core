/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import { sendWithPromise, addWebUiListener } from 'chrome://resources/js/cr.js'

export class Scriptlet {
  name: string
  kind: object = {
    mime: 'application/javascript'
  }
  content: string
}

export enum ErrorCode {
  kOK = 0,
  kInvalidName,
  kAlreadyExists,
  kNotFound,
}

export interface BraveAdblockBrowserProxy {
  getRegionalLists(): Promise<any[]> // TODO(petemill): Define the expected type
  enableFilterList(uuid: string, enabled: boolean)
  updateFilterList(uuid: string): Promise<boolean>
  getListSubscriptions(): Promise<any> // TODO(petemill): Define the expected type
  getCustomFilters(): Promise<any> // TODO(petemill): Define the expected type
  setSubscriptionEnabled(url: string, enabled: boolean)
  addSubscription(url: string)
  addWebUiListener(eventName: string, callback: Function)
  updateSubscription(url: string)
  deleteSubscription(url: string)
  viewSubscription(url: string)
  getCustomScriptlets(): Promise<Scriptlet[]>
  addCustomScriptlet(scriptlet: Scriptlet): Promise<ErrorCode>
  updateCustomScriptlet(name: string, scriptlet: Scriptlet): Promise<ErrorCode>
  removeCustomScriptlet(name: string): Promise<ErrorCode>
}

export class BraveAdblockBrowserProxyImpl implements BraveAdblockBrowserProxy {
  static getInstance() {
    return instance || (instance = new BraveAdblockBrowserProxyImpl());
  }

  /** @returns {Promise} */
  getRegionalLists () {
    return sendWithPromise('brave_adblock.getRegionalLists')
  }

  /** @returns {Promise} */
  getListSubscriptions () {
    return sendWithPromise('brave_adblock.getListSubscriptions')
  }

  /** @returns {Promise} */
  getCustomFilters () {
    return sendWithPromise('brave_adblock.getCustomFilters')
  }

  enableFilterList (uuid, enabled) {
    chrome.send('brave_adblock.enableFilterList', [uuid, enabled])
  }

  /** @returns {Promise<boolean>} */
  updateFilterLists () {
    return sendWithPromise('brave_adblock.updateFilterLists')
  }

  setSubscriptionEnabled (url, enabled) {
    chrome.send('brave_adblock.setSubscriptionEnabled', [url, enabled])
  }

  addSubscription (url) {
    chrome.send('brave_adblock.addSubscription', [url])
  }

  updateSubscription (url) {
    chrome.send('brave_adblock.updateSubscription', [url])
  }

  updateCustomFilters (value) {
    chrome.send('brave_adblock.updateCustomFilters', [value])
  }

  deleteSubscription (url) {
    chrome.send('brave_adblock.deleteSubscription', [url])
  }

  viewSubscription (url) {
    chrome.send('brave_adblock.viewSubscription', [url])
  }

  utf8ToBase64_(str) {
    const uint8Array = new TextEncoder().encode(str)
    const base64String = btoa(String.fromCharCode.apply(null, uint8Array))
    return base64String
  }

  base64ToUtf8_(base64) {
    const binaryString = atob(base64)
    const bytes = new Uint8Array(binaryString.length)
    for (let i = 0; i < binaryString.length; i++) {
      bytes[i] = binaryString.charCodeAt(i)
    }
    return new TextDecoder().decode(bytes)
  }

  getCustomScriptlets() {
    return sendWithPromise('brave_adblock.getCustomScriptlets')
      .then((scriptlets) => {
        for (const scriptlet of scriptlets) {
          scriptlet.content = this.base64ToUtf8_(scriptlet.content)
        }
        return scriptlets
      })
      .catch((error) => {
        throw error
      })
  }

  addCustomScriptlet(scriptlet) {
    scriptlet.content = this.utf8ToBase64_(scriptlet.content)
    return sendWithPromise('brave_adblock.addCustomScriptlet', scriptlet)
  }

  updateCustomScriptlet(name, scriptlet) {
    scriptlet.content = this.utf8ToBase64_(scriptlet.content)
    return sendWithPromise(
      'brave_adblock.updateCustomScriptlet',
      name,
      scriptlet
    )
  }

  removeCustomScriptlet(name) {
    return sendWithPromise('brave_adblock.removeCustomScriptlet', name)
  }

  addWebUiListener(event_name, callback) {
    addWebUiListener(event_name, callback)
  }
}

let instance: BraveAdblockBrowserProxy|null = null
