/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Tab } from '../../types/state/shieldsPannelState'
import { BlockOptions } from '../../types/other/blockTypes'
import * as resourceIdentifiers from '../../constants/resourceIdentifiers'

/**
 * Obtains the shields panel data for the specified tab data
 * @param {Object} tabData the details of the tab
 * @return a promise with the corresponding shields panel data for the input tabData
 */
export const getShieldSettingsForTabData = (tabData?: chrome.tabs.Tab) => {
  if (tabData === undefined || !tabData.url) {
    return Promise.reject(new Error('No tab url specified'))
  }

  const url = new window.URL(tabData.url)
  const origin = url.origin
  const hostname = url.hostname

  return Promise.all([
    chrome.contentSettings.plugins.getAsync({ primaryUrl: origin, resourceIdentifier: { id: resourceIdentifiers.RESOURCE_IDENTIFIER_BRAVE_SHIELDS } }),
    chrome.contentSettings.plugins.getAsync({ primaryUrl: origin, resourceIdentifier: { id: resourceIdentifiers.RESOURCE_IDENTIFIER_ADS } }),
    chrome.contentSettings.plugins.getAsync({ primaryUrl: origin, resourceIdentifier: { id: resourceIdentifiers.RESOURCE_IDENTIFIER_TRACKERS } }),
    chrome.contentSettings.plugins.getAsync({ primaryUrl: origin, resourceIdentifier: { id: resourceIdentifiers.RESOURCE_IDENTIFIER_HTTP_UPGRADABLE_RESOURCES } }),
    chrome.contentSettings.javascript.getAsync({ primaryUrl: origin }),
    chrome.contentSettings.plugins.getAsync({ primaryUrl: origin }),
    chrome.contentSettings.plugins.getAsync({ primaryUrl: origin, secondaryUrl: 'https://firstParty/*', resourceIdentifier: { id: resourceIdentifiers.RESOURCE_IDENTIFIER_FINGERPRINTING } }),
    chrome.contentSettings.plugins.getAsync({ primaryUrl: origin, resourceIdentifier: { id: resourceIdentifiers.RESOURCE_IDENTIFIER_COOKIES } }),
    chrome.contentSettings.plugins.getAsync({ primaryUrl: origin, secondaryUrl: 'https://firstParty/', resourceIdentifier: { id: resourceIdentifiers.RESOURCE_IDENTIFIER_COOKIES } })
  ]).then((details) => {
    const fingerprinting = details[5].setting !== details[6].setting ? 'block_third_party' : details[5].setting
    const cookies = details[7].setting !== details[8].setting ? 'block_third_party' : details[7].setting
    return {
      url: url.href,
      origin,
      hostname,
      id: tabData.id,
      braveShields: details[0].setting,
      ads: details[1].setting,
      trackers: details[2].setting,
      httpUpgradableResources: details[3].setting,
      javascript: details[4].setting,
      fingerprinting,
      cookies
    }
  }).catch(() => {
    return {
      url: url.href,
      origin,
      hostname,
      id: tabData.id,
      ads: 0,
      trackers: 0,
      httpUpgradableResources: 0,
      javascript: 0,
      fingerprinting: 0
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
    .then((details: Partial<Tab>) => {
      const actions = require('../actions/shieldsPanelActions')
      actions.default.shieldsPanelDataUpdated(details)
    })

/**
 * Changes the brave shields setting at origin to be allowed or blocked.
 * @param {string} origin the origin of the site to change the setting for
 * @param {string} setting 'allow' or 'block'
 * @return a promise which resolves when the setting is set
 */
export const setAllowBraveShields = (origin: string, setting: string) =>
  chrome.contentSettings.plugins.setAsync({
    primaryPattern: origin.replace(/^(http|https):\/\//, '*://') + '/*',
    resourceIdentifier: { id: resourceIdentifiers.RESOURCE_IDENTIFIER_BRAVE_SHIELDS },
    setting
  })

/**
 * Changes the ads at origin to be allowed or blocked.
 * The ad-block service will come into effect if the ad is marked as blocked.
 * @param {string} origin the origin of the site to change the setting for
 * @param {string} setting 'allow' or 'block'
 * @return a promise which resolves when the setting is set
 */
export const setAllowAds = (origin: string, setting: string) =>
  chrome.contentSettings.plugins.setAsync({
    primaryPattern: origin + '/*',
    resourceIdentifier: { id: resourceIdentifiers.RESOURCE_IDENTIFIER_ADS },
    setting
  })

/**
 * Changes the trackers at origin to be allowed or blocked.
 * The tracking-protection service will come into effect if the tracker is marked as blocked.
 * @param {string} origin the origin of the site to change the setting for
 * @param {string} setting 'allow' or 'block'
 * @return a promise which resolves with the setting is set
 */
export const setAllowTrackers = (origin: string, setting: string) =>
  chrome.contentSettings.plugins.setAsync({
    primaryPattern: origin + '/*',
    resourceIdentifier: { id: resourceIdentifiers.RESOURCE_IDENTIFIER_TRACKERS },
    setting
  })

/**
 * Changes the http upgrdabable resources to be allows as is or blocked.
 * The https-everywhere service will come into effect if the resource is marked as blocked.
 * @param {string} origin the origin of the site to change the setting for
 * @return a promise which resolves when the setting is set
 */
export const setAllowHTTPUpgradableResources = (origin: string, setting: BlockOptions) => {
  const primaryPattern = origin.replace(/^(http|https):\/\//, '*://') + '/*'
  return chrome.contentSettings.plugins.setAsync({
    primaryPattern,
    resourceIdentifier: { id: resourceIdentifiers.RESOURCE_IDENTIFIER_HTTP_UPGRADABLE_RESOURCES },
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
 * Changes the fingerprinting at origin to be allowed or blocked.
 * The fingerprinting-protection service will come into effect if the fingerprinting is marked as blocked.
 * @param {string} origin the origin of the site to change the setting for
 * @return a promise which resolves with the setting is set
 */
export const setAllowFingerprinting = (origin: string, setting: string) => {
  const originSetting = setting === 'allow' ? 'allow' : 'block'
  const firstPartySetting = setting === 'block' ? 'block' : 'allow'

  const p1 = chrome.contentSettings.plugins.setAsync({
    primaryPattern: origin + '/*',
    resourceIdentifier: { id: resourceIdentifiers.RESOURCE_IDENTIFIER_FINGERPRINTING },
    setting: originSetting
  })

  const p2 = chrome.contentSettings.plugins.setAsync({
    primaryPattern: origin + '/*',
    secondaryPattern: 'https://firstParty/*',
    resourceIdentifier: { id: resourceIdentifiers.RESOURCE_IDENTIFIER_FINGERPRINTING },
    setting: firstPartySetting
  })

  return Promise.all([p1, p2])
}

/**
 * Changes the cookie at origin to be allowed or blocked.
 * @param {string} origin the origin of the site to change the setting for
 * @return a promise which resolves with the setting is set
 */
export const setAllowCookies = (origin: string, setting: string) => {
  const originSetting = setting === 'allow' ? 'allow' : 'block'
  const firstPartySetting = setting === 'block' ? 'block' : 'allow'

  const p1 = chrome.contentSettings.plugins.setAsync({
    primaryPattern: origin + '/*',
    resourceIdentifier: { id: resourceIdentifiers.RESOURCE_IDENTIFIER_REFERRERS },
    setting: originSetting
  })

  const p2 = chrome.contentSettings.plugins.setAsync({
    primaryPattern: origin + '/*',
    resourceIdentifier: { id: resourceIdentifiers.RESOURCE_IDENTIFIER_COOKIES },
    setting: originSetting
  })

  const p3 = chrome.contentSettings.plugins.setAsync({
    primaryPattern: origin + '/*',
    secondaryPattern: 'https://firstParty/*',
    resourceIdentifier: { id: resourceIdentifiers.RESOURCE_IDENTIFIER_COOKIES },
    setting: firstPartySetting
  })

  return Promise.all([p1, p2, p3])
}

/**
 * Toggles the input value between allow and block
 * @return the toggled value
 */
export const toggleShieldsValue = (value: BlockOptions) =>
  value === 'allow' ? 'block' : 'allow'
