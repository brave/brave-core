/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Utils
import { debounce } from '../common/debounce'

const keyName = 'new-tab-data'

const defaultState: NewTab.State = {
  initialDataLoaded: false,
  textDirection: window.loadTimeData.getString('textdirection'),
  featureFlagBraveNTPBrandedWallpaper: window.loadTimeData.getBoolean('featureFlagBraveNTPBrandedWallpaper'),
  showBackgroundImage: false,
  showStats: false,
  showClock: false,
  showTopSites: false,
  showRewards: false,
  brandedWallpaperOptIn: false,
  isBrandedWallpaperNotificationDismissed: true,
  topSites: [],
  ignoredTopSites: [],
  pinnedTopSites: [],
  gridSites: [],
  showEmptyPage: false,
  isIncognito: chrome.extension.inIncognitoContext,
  useAlternativePrivateSearchEngine: false,
  isTor: false,
  isQwant: false,
  bookmarks: {},
  stats: {
    adsBlockedStat: 0,
    javascriptBlockedStat: 0,
    httpsUpgradesStat: 0,
    fingerprintingBlockedStat: 0,
    bandwidthSavedStat: 0
  },
  rewardsState: {
    adsEstimatedEarnings: 0,
    balance: {
      total: 0,
      rates: {},
      wallets: {}
    },
    dismissedNotifications: [],
    enabledAds: false,
    adsSupported: false,
    enabledMain: false,
    promotions: [],
    onlyAnonWallet: false,
    totalContribution: 0.0,
    walletCreated: false,
    walletCreating: false,
    walletCreateFailed: false,
    walletCorrupted: false
  }
}

if (chrome.extension.inIncognitoContext) {
  defaultState.isTor = window.loadTimeData.getBoolean('isTor')
  defaultState.isQwant = window.loadTimeData.getBoolean('isQwant')
}

const getPersistentData = (state: NewTab.State): NewTab.PersistentState => {
  // Don't save items which we aren't the source
  // of data for.
  const peristantState: NewTab.PersistentState = {
    topSites: state.topSites,
    ignoredTopSites: state.ignoredTopSites,
    pinnedTopSites: state.pinnedTopSites,
    gridSites: state.gridSites,
    showEmptyPage: state.showEmptyPage,
    bookmarks: state.bookmarks,
    rewardsState: state.rewardsState
  }

  return peristantState
}

const cleanData = (state: NewTab.State) => {
  // We need to disable linter as we defined in d.ts that this values are number,
  // but we need this check to covert from old version to a new one
  /* tslint:disable */
  if (typeof state.rewardsState.adsEstimatedEarnings === 'string') {
    state.rewardsState.adsEstimatedEarnings = 0.0
  }

  if (typeof state.rewardsState.totalContribution === 'string') {
    state.rewardsState.totalContribution = 0.0
  }
  /* tslint:enable */

  return state
}

export const load = (): NewTab.State => {
  const data: string | null = window.localStorage.getItem(keyName)
  let state = defaultState
  let storedState: NewTab.PersistentState
  if (data) {
    try {
      storedState = JSON.parse(data)
      // add defaults for non-peristant data
      state = {
        ...state,
        ...storedState
      }
    } catch (e) {
      console.error('Could not parse local storage data: ', e)
    }
  }
  return cleanData(state)
}

export const debouncedSave = debounce<NewTab.State>((data: NewTab.State) => {
  if (data) {
    const dataToSave = getPersistentData(data)
    window.localStorage.setItem(keyName, JSON.stringify(dataToSave))
  }
}, 50)
