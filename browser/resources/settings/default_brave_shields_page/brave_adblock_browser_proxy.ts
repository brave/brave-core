/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import { sendWithPromise, addWebUIListener } from 'chrome://resources/js/cr.m.js';

export interface BraveAdblockBrowserProxy {
  getRegionalLists(): Promise<any[]> // TODO(petemill): Define the expected type
  enableFilterList(uuid: string, enabled: boolean)
  getListSubscriptions(): Promise<any> // TODO(petemill): Define the expected type
  getCustomFilters(): Promise<any> // TODO(petemill): Define the expected type
  setSubscriptionEnabled(url: string, enabled: boolean)
  addSubscription(url: string)
  addWebUIListener(eventName: string, callback: Function)
  updateSubscription(url: string)
  deleteSubscription(url: string)
  viewSubscription(url: string)
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

  addWebUIListener (event_name, callback) {
    addWebUIListener(event_name, callback)
  }
}

let instance: BraveAdblockBrowserProxy|null = null
