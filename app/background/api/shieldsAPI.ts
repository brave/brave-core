/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Tab } from '../../types/state/shieldsPannelState'
import { BlockOptions } from '../../types/other/blockTypes'

/**
 * Obtains the shields panel data for the specified tab data
 * @param {Object} tabData the details of the tab
 * @return a promise with the corresponding shields panel data for the input tabData
 */
export const getShieldSettingsForTabData = (tabData: chrome.tabs.Tab) => {
  if (!tabData.url) {
    return Promise.reject(new Error('No tab url specified'))
  }

  const url = new window.URL(tabData.url)
  const origin = url.origin
  const hostname = url.hostname

  return Promise.all([
    chrome.contentSettings.braveAdBlock.getAsync({ primaryUrl: origin }),
    chrome.contentSettings.braveTrackingProtection.getAsync({ primaryUrl: origin }),
    chrome.contentSettings.braveHTTPSEverywhere.getAsync({ primaryUrl: origin }),
    chrome.contentSettings.javascript.getAsync({ primaryUrl: origin })
  ]).then((details) => {
    return {
      url: url.href,
      origin,
      hostname,
      id: tabData.id,
      adBlock: details[0].setting,
      trackingProtection: details[1].setting,
      httpsEverywhere: details[2].setting,
      javascript: details[3].setting
    }
  }).catch(() => {
    return {
      url: url.href,
      origin,
      hostname,
      id: tabData.id,
      adBlock: 0,
      trackingProtection: 0,
      httpsEverywhere: 0,
      javascript: 0
    }
  })
}

/**
 * Obtains specified tab data
 * @return a promise with the active tab data
 */
export const getTabData = (tabId: number) =>
  chrome.tabs.getAsync(tabId)

/**
 * Obtains new information about the shields panel settings for the specified tabId
 * @param {number} tabId the tabId of the tab who's content settings are of interest
 * @return a promise which resolves with the updated shields panel data.
 */
export const requestShieldPanelData = (tabId: number) =>
  getTabData(tabId)
    .then(getShieldSettingsForTabData)
    .then((details: Tab) => {
      const actions = require('../actions/shieldsPanelActions')
      actions.shieldsPanelDataUpdated(details)
    })

/**
 * Changes the ad block to be on (allow) or off (block)
 * @param {string} origin the origin of the site to change the setting for
 * @param {string} setting 'allow' or 'block'
 * @return a promise which resolves when the setting is set
 */
export const setAllowAdBlock = (origin: string, setting: string) =>
  chrome.contentSettings.braveAdBlock.setAsync({
    primaryPattern: origin + '/*',
    setting
  })

/**
 * Changes the tracking protection to be on (allow) or off (block)
 * @param {string} origin the origin of the site to change the setting for
 * @param {string} setting 'allow' or 'block'
 * @return a promise which resolves with the setting is set
 */
export const setAllowTrackingProtection = (origin: string, setting: string) =>
  chrome.contentSettings.braveTrackingProtection.setAsync({
    primaryPattern: origin + '/*',
    setting
  })

/**
 * Changes the HTTPS Everywhere to be on (allow) or off (block)
 * @param {string} origin the origin of the site to change the setting for
 * @return a promise which resolves when the setting is set
 */
export const setAllowHTTPSEverywhere = (origin: string, setting: BlockOptions) => {
  const primaryPattern = origin.replace(/^(http|https):\/\//, '*://') + '/*'
  return chrome.contentSettings.braveHTTPSEverywhere.setAsync({
    primaryPattern,
    setting
  })
}

/**
 * Changes the Javascript to be on (allow) or off (block)
 * @param {string} origin the origin of the site to change the setting for
 * @param {string} setting 'allow' or 'block'
 * @return a promise which resolves when the setting is set
 */
export const setAllowJavaScript = (origin: string, setting: string) =>
  chrome.contentSettings.javascript.setAsync({
    primaryPattern: origin + '/*',
    setting
  })

/**
 * Toggles the input value between allow and block
 * @return the toggled value
 */
export const toggleShieldsValue = (value: BlockOptions) =>
  value === 'allow' ? 'block' : 'allow'
