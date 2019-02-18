/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * Creates a tab with the specified createProperties
 * @param {Object} createProperties as per the chrome extension API
 * @return a promise which resolves when the tab is created.
 */
export const createTab = (createProperties: chrome.tabs.CreateProperties) =>
  new Promise<chrome.tabs.Tab>((resolve) => {
    chrome.tabs.create(createProperties, (tab) => {
      resolve(tab)
    })
  })

/**
 * Reloads the specified tab
 * @param {number} tabId the tab ID
 * @param {boolean} byPassCache true if the cache should be bypassed
 * @return a promise which resolves when the tab is reloaded.
 */
export const reloadTab = (tabId: number, bypassCache: boolean) =>
  new Promise<void>((resolve) => {
    chrome.tabs.reload(tabId, { bypassCache }, () => {
      resolve()
    })
  })
