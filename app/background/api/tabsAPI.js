/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * Creates a tab with the specified createProperties
 * @param {Object} createProperties as per the chrome extension API
 * @return a promise which resolves when the callback is called
 */
export const createTab = (createProperties) =>
  new Promise((resolve) => {
    chrome.tabs.create(createProperties, (tab) => {
      resolve(tab)
    })
  })
