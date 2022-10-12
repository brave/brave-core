/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

import { sendWithPromise, addWebUIListener } from 'chrome://resources/js/cr.m.js';

/** @interface */
export class BraveAdblockBrowserProxy {
  getInstance() { }
  getRegionalLists() { }
  enableFilterList() { }
  getListSubscriptions() { }
  getCustomFilters() { }
  setSubscriptionEnabled() { }
  addSubscription() { }
  addWebUIListener() { }
  updateSubscription() { }
  deleteSubscription() { }
  viewSubscription() { }
}

/**
* @implements {BraveAdblockBrowserProxy}
*/
export class BraveAdblockBrowserProxyImpl {
  /** @instance */
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

/** @type {BraveAdblockBrowserProxyImpl} */
let instance
