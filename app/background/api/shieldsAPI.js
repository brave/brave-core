/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import actions from '../actions/shieldsPanelActions'

/**
 * Obtains the shields panel data for the specified tab data
 * @param {Object} tabData the details of the tab
 * @return a promise with the corresponding shields panel data for the input tabData
 */
export const getShieldSettingsForTabData = (tabData) => {
  if (tabData == null) {
    return Promise.reject(new Error('No tab specified'))
  }
  const url = new window.URL(tabData.url)
  const origin = url.origin
  const hostname = url.hostname
  return Promise.all([
    chrome.contentSettings.braveAdBlock.getAsync({primaryUrl: origin}),
    chrome.contentSettings.braveTrackingProtection.getAsync({primaryUrl: origin})
  ]).then((details) => {
    return {
      origin,
      hostname,
      adBlock: details[0].setting,
      trackingProtection: details[1].setting,
      tabId: tabData.id
    }
  })
}

/**
 * Obtains the active tab data
 * @return a promise with the active tab data
 */
export const getActiveTabData = () => {
  return chrome.tabs.queryAsync({'active': true, 'lastFocusedWindow': true})
    .then((tabs) => {
      return (tabs.length && tabs[0]) || undefined
    })
}

/**
 * Updates the shields settings from the active tab
 * @return a promise which resolves with the updated shields panel data.
 */
export const updateShieldsSettings = () =>
  getActiveTabData()
    .then(getShieldSettingsForTabData)
    .then((details) => {
      actions.shieldsPanelDataUpdated(details)
    })

/**
 * Changes the ad block to be on (allow) or off (block)
 * @param {string} origin the origin of the site to change the setting for
 * @param {string} setting 'allow' or 'block'
 * @return a promise which resolves with the updated shields panel data.
 */
export const setAllowAdBlock = (origin, setting) => {
  return chrome.contentSettings.braveAdBlock.setAsync({
    primaryPattern: origin + '/*',
    setting
  }).then(updateShieldsSettings)
}

/**
 * Changes the tracking protection to be on (allow) or off (block)
 * @param {string} origin the origin of the site to change the setting for
 * @param {string} setting 'allow' or 'block'
 * @return a promise which resolves with the updated shields panel data.
 */
export const setAllowTrackingProtection = (origin, setting) => {
  return chrome.contentSettings.braveTrackingProtection.setAsync({
    primaryPattern: origin + '/*',
    setting
  }).then(updateShieldsSettings)
}

/**
 * Toggles the input value between allow and block
 * @return the toggled value
 */
export const toggleShieldsValue = (value) =>
  value === 'allow' ? 'block' : 'allow'
