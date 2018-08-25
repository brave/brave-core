/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { isHttpOrHttps } from '../../helpers/urlUtils'

export const shieldsOnIcon = 'img/icon.svg'
export const shieldsOffIcon = 'img/icon-off.svg'

/**
 * Initializes the browser action UI
 */
export function init () {
  // Setup badge color
  chrome.browserAction.setBadgeBackgroundColor({
    color: [66, 66, 66, 100]
  })
  // Initial / default icon
  chrome.browserAction.setIcon({
    path: shieldsOnIcon
  })
  // By default, icon is disabled,
  // so that we do not enable the icon in a new tab and then disable it
  // when the context is not http(s).
  chrome.browserAction.disable()
}

/**
 * Sets the badge text
 * @param {string} text - The text to put on the badge
 */
export const setBadgeText = (tabId: number, text: string) => {
  if (chrome.browserAction) {
    chrome.browserAction.setBadgeText({
      tabId,
      text: String(text)
    })
  }
}

/**
 * Updates the shields icon based on shields state
 */
export const setIcon = (url: string, tabId: number, shieldsOn: boolean) => {

  const actionIsDisabled = !isHttpOrHttps(url)
  if (chrome.browserAction) {
    chrome.browserAction.setIcon({
      path: shieldsOn ? shieldsOnIcon : shieldsOffIcon,
      tabId
    })
    if (actionIsDisabled) {
      chrome.browserAction.disable(tabId)
    } else {
      chrome.browserAction.enable(tabId)
    }
  }
}
