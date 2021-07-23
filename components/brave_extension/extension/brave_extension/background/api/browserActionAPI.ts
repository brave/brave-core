/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { isHttpOrHttps } from '../../helpers/urlUtils'

// TODO: use `import` for these assets once the webpack build
// for brave extension allows dynamic file serving like other brave-core
// components
const shieldsOnIcon18Url = 'assets/img/shields-on.png'
const shieldsOnIcon36Url = 'assets/img/shields-on@2x.png'
const shieldsOnIcon54Url = 'assets/img/shields-on@3x.png'
const shieldsOffIcon18Url = 'assets/img/shields-off.png'
const shieldsOffIcon36Url = 'assets/img/shields-off@2x.png'
const shieldsOffIcon54Url = 'assets/img/shields-off@3x.png'

export const shieldsOnIcon = {
  18: shieldsOnIcon18Url,
  36: shieldsOnIcon36Url,
  54: shieldsOnIcon54Url
}
export const shieldsOffIcon = {
  18: shieldsOffIcon18Url,
  36: shieldsOffIcon36Url,
  54: shieldsOffIcon54Url
}

/**
 * Initializes the browser action UI
 */
export async function init () {
  // Setup badge color
  try {
    await chrome.browserAction.setBadgeBackgroundColor({
      color: '#636473'
    })
  } catch (e) {
    console.error(e)
  }
  // Initial / default icon
  chrome.browserAction.setIcon({
    path: shieldsOnIcon
  })
  // By default, icon is disabled,
  // so that we do not enable the icon in a new tab and then disable it
  // when the context is not http(s).
  try {
    await chrome.browserAction.disable()
  } catch (e) {
    console.error(e)
  }
}

/**
 * Sets the badge text
 * @param {string} text - The text to put on the badge
 */
export const setBadgeText = async (tabId: number, text: string) => {
  if (chrome.browserAction) {
    try {
      await chrome.browserAction.setBadgeText({
        tabId,
        text: String(text)
      })
    } catch (e) {
      console.error(e)
    }
  }
}

/**
 * Updates the shields icon based on shields state
 */
export const setIcon = async (url: string, tabId: number, shieldsOn: boolean) => {

  const actionIsDisabled = !isHttpOrHttps(url)
  if (chrome.browserAction) {
    chrome.browserAction.setIcon({
      path: shieldsOn ? shieldsOnIcon : shieldsOffIcon,
      tabId
    })
    try {
      if (actionIsDisabled) {
        await chrome.browserAction.disable(tabId)
      } else {
        await chrome.browserAction.enable(tabId)
      }
    } catch (e) {
      console.error(e)
    }
  }
}
