/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { ShieldDetails } from '../../types/actions/shieldsPanelActions'
import { BlockOptions } from '../../types/other/blockTypes'
import actions from '../actions/shieldsPanelActions'
import * as SettingsPrivate from '../../../../../common/settingsPrivate'

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
    chrome.braveShields.getBraveShieldsEnabledAsync(tabData.url),
    chrome.braveShields.getAdControlTypeAsync(tabData.url),
    chrome.braveShields.isFirstPartyCosmeticFilteringEnabledAsync(tabData.url),
    chrome.braveShields.getHTTPSEverywhereEnabledAsync(tabData.url),
    chrome.braveShields.getNoScriptControlTypeAsync(tabData.url),
    chrome.braveShields.getFingerprintingControlTypeAsync(tabData.url),
    chrome.braveShields.getCookieControlTypeAsync(tabData.url)
  ]).then((details) => {
    return {
      url: url.href,
      origin,
      hostname,
      id: tabData.id,
      braveShields: details[0] ? 'allow' : 'block',
      ads: details[1],
      trackers: details[1],
      firstPartyCosmeticFiltering: details[2],
      httpUpgradableResources: details[3] ? 'block' : 'allow',
      javascript: details[4],
      fingerprinting: details[5],
      cookies: details[6]
    }
  }).catch(() => {
    return {
      url: url.href,
      origin,
      hostname,
      id: tabData.id,
      braveShields: 'block',
      ads: 0,
      trackers: 0,
      httpUpgradableResources: 0,
      javascript: 0,
      fingerprinting: 0
    }
  })
}

/**
 * Filters ipfs and ipns locations to their equivalent gateway
 * @return a promise with the possibly modified tab data
 */
const filterTabData = (tabData?: chrome.tabs.Tab) => {
  if (tabData === undefined || !tabData.url) {
    return Promise.reject(new Error('No tab url specified'))
  }
  const url = new window.URL(tabData.url)
  return new Promise(resolve => {
    if ((url.protocol === 'ipfs:' || url.protocol === 'ipns:') && tabData.url) {
      chrome.ipfs.resolveIPFSURI(tabData.url, (gatewayUrl: string) => {
        tabData.url = gatewayUrl
        resolve(tabData)
      })
      return
    }
    resolve(tabData)
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
    .then(filterTabData)
    .then(getShieldSettingsForTabData)
    .then((details: ShieldDetails) => {
      actions.shieldsPanelDataUpdated(details)
    })
    .catch((error: any) =>
      console.error('[Shields]: Can\'t request shields panel data for tabId: ' + tabId + '. ', error))

/**
 * Changes the brave shields setting at origin to be allowed or blocked.
 * @param {string} origin the origin of the site to change the setting for
 * @param {string} setting 'allow' or 'block'
 * @return a promise which resolves when the setting is set
 */
export const setAllowBraveShields = (origin: string, setting: string) =>
  chrome.braveShields.setBraveShieldsEnabledAsync(setting === 'allow' ? true : false, origin)

/**
 * Changes the ads at origin to be allowed or blocked.
 * The ad-block service will come into effect if the ad is marked as blocked.
 * @param {string} origin the origin of the site to change the setting for
 * @param {string} setting 'allow' or 'block'
 * @return a promise which resolves when the setting is set
 */
export const setAllowAds = (origin: string, setting: string) =>
  chrome.braveShields.setAdControlTypeAsync(setting, origin)

/**
 * Changes the trackers at origin to be allowed or blocked.
 * The tracking-protection service will come into effect if the tracker is marked as blocked.
 * @param {string} origin the origin of the site to change the setting for
 * @param {string} setting 'allow' or 'block'
 * @return a promise which resolves with the setting is set
 */
export const setAllowTrackers = (origin: string, setting: string) => {
  return chrome.braveShields.setAdControlTypeAsync(setting, origin)
}

/**
 * Changes the cosmetic filtering at origin to be allowed or blocked.
 * The tracking-protection service will come into effect if the tracker is marked as blocked.
 * @param {string} origin the origin of the site to change the setting for
 * @param {string} setting 'allow', 'block_third_party', or 'block'
 * @return a promise which resolves with the setting is set
 */
export const setAllowCosmeticFiltering = (origin: string, setting: string) => {
  return chrome.braveShields.setCosmeticFilteringControlTypeAsync(setting, origin)
}

/**
 * Changes the http upgrdabable resources to be allows as is or blocked.
 * The https-everywhere service will come into effect if the resource is marked as blocked.
 * @param {string} origin the origin of the site to change the setting for
 * @return a promise which resolves when the setting is set
 */
export const setAllowHTTPUpgradableResources = (origin: string, setting: BlockOptions) =>
  chrome.braveShields.setHTTPSEverywhereEnabledAsync(setting === 'allow' ? false : true, origin)

/**
 * Changes the Javascript to be on (allow) or off (block)
 * @param {string} origin the origin of the site to change the setting for
 * @param {string} setting 'allow' or 'block'
 * @return a promise which resolves when the setting is set
 */
export const setAllowJavaScript = (origin: string, setting: string) =>
  chrome.braveShields.setNoScriptControlTypeAsync(setting, origin)

/**
 * Changes the fingerprinting at origin to be allowed or blocked.
 * The fingerprinting-protection service will come into effect if the fingerprinting is marked as blocked.
 * @param {string} origin the origin of the site to change the setting for
 * @return a promise which resolves with the setting is set
 */
export const setAllowFingerprinting = (origin: string, setting: string) =>
  chrome.braveShields.setFingerprintingControlTypeAsync(setting, origin)

/**
 * Changes the cookie at origin to be allowed or blocked.
 * @param {string} origin the origin of the site to change the setting for
 * @return a promise which resolves with the setting is set
 */
export const setAllowCookies = (origin: string, setting: string) =>
  chrome.braveShields.setCookieControlTypeAsync(setting, origin)

/**
 * Toggles the input value between allow and block
 * @return the toggled value
 */
export const toggleShieldsValue = (value: BlockOptions) =>
  value === 'allow' ? 'block' : 'allow'

/**
 * Temporary allow a set of script origins for a specific tab until reload.
 * @param {Array<string>} origins a set of script origins to be allowed
 * @param {number} tabId ID of the tab which these origins are allowed in
 * @return a promise which resolves when the origins are set.
 */
export const setAllowScriptOriginsOnce = (origins: Array<string>, tabId: number) =>
  new Promise<void>((resolve) => {
    chrome.braveShields.allowScriptsOnce(origins, tabId, () => {
      resolve()
    })
  })

/**
 * Open a prompt that allows the user to submit a report telling Brave that the current website is broken by Shields.
 * @param {number} tabId ID of the tab whose contents are being reported
 */
export const reportBrokenSite = (tabId: number) =>
  chrome.braveShields.reportBrokenSite(tabId)

export type GetViewPreferencesData = {
  showAdvancedView: boolean
  statsBadgeVisible: boolean
}

const settingsKeys = {
  showAdvancedView: { key: 'brave.shields.advanced_view_enabled', type: chrome.settingsPrivate.PrefType.BOOLEAN },
  statsBadgeVisible: { key: 'brave.shields.stats_badge_visible', type: chrome.settingsPrivate.PrefType.BOOLEAN }
}
export async function getViewPreferences (): Promise<GetViewPreferencesData> {
  let newSettings = {} as GetViewPreferencesData
  await Promise.all(
    Object.keys(settingsKeys).map(async (name) => {
      // Get setting by internal key
      const pref = await SettingsPrivate.getPreference(settingsKeys[name].key)
      // Validate setting type
      if (pref.type !== settingsKeys[name].type) {
        throw new Error(`Unexpected settings type received for "${settingsKeys[name].key}". Expected: ${settingsKeys[name].type}, Received: ${pref.type}`)
      }
      // Valid
      newSettings[name] = pref.value
    })
  )
  return newSettings
}

export type SetViewPreferencesData = {
  showAdvancedView?: boolean
  statsBadgeVisible?: boolean
}
export async function setViewPreferences (preferences: SetViewPreferencesData): Promise<void> {
  const setOps = []
  if (preferences.showAdvancedView !== undefined) {
    setOps.push(
      SettingsPrivate.setPreference(settingsKeys.showAdvancedView.key, preferences.showAdvancedView)
    )
  }
  if (preferences.statsBadgeVisible !== undefined) {
    setOps.push(
      SettingsPrivate.setPreference(settingsKeys.statsBadgeVisible.key, preferences.statsBadgeVisible)
    )
  }
  await Promise.all(setOps)
}

export const onShieldsPanelShown = () =>
  new Promise<void>((resolve) => {
    chrome.braveShields.onShieldsPanelShown()
  })
