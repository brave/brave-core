/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { isHttpOrHttps } from '../../helpers/urlUtils'

/**
 * Sets the badge text
 * @param {string} text - The text to put on the badge
 */
export const setBadgeText = (text: string) => {
  if (chrome.browserAction) {
    chrome.browserAction.setBadgeText({ text: String(text) })
  }
}

/**
 * Updates the shields icon based on shields state
 */
export const setIcon = (url: string, tabId: number, shieldsOn: boolean) => {
  const shieldsEnabledIcon = 'img/icon-16.png'
  const shieldsDisabledIcon = 'img/icon-16-disabled.png'

  if (chrome.browserAction) {
    chrome.browserAction.setIcon({
      path: shieldsOn && isHttpOrHttps(url) ? shieldsEnabledIcon : shieldsDisabledIcon,
      tabId
    })
  }
}
