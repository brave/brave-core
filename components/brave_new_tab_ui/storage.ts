/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as backgroundAPI from './api/background'

// Utils
import { debounce } from '../common/debounce'

const keyName = 'new-tab-data'

const defaultState: NewTab.State = {
  topSites: [],
  ignoredTopSites: [],
  pinnedTopSites: [],
  gridSites: [],
  showEmptyPage: false,
  isIncognito: chrome.extension.inIncognitoContext,
  useAlternativePrivateSearchEngine: false,
  isTor: chrome.getVariableValue('isTor') === 'true',
  isQwant: chrome.getVariableValue('isQwant') === 'true',
  bookmarks: {},
  stats: {
    adsBlockedStat: 0,
    trackersBlockedStat: 0,
    javascriptBlockedStat: 0,
    httpsUpgradesStat: 0,
    fingerprintingBlockedStat: 0
  }
}

export const getLoadTimeData = (state: NewTab.State) => {
  state = { ...state }
  state.stats = defaultState.stats
  ;['adsBlockedStat', 'trackersBlockedStat', 'javascriptBlockedStat',
    'httpsUpgradesStat', 'fingerprintingBlockedStat'].forEach((stat) => {
      state.stats[stat] = parseInt(chrome.getVariableValue(stat), 10) || 0
    })
  state.useAlternativePrivateSearchEngine = chrome.getVariableValue('useAlternativePrivateSearchEngine') === 'true'
  return state
}

const cleanData = (state: NewTab.State): NewTab.State => {
  state = { ...state }
  state.backgroundImage = backgroundAPI.randomBackgroundImage()
  state = getLoadTimeData(state)
  return state
}

export const load = (): NewTab.State => {
  const data = window.localStorage.getItem(keyName)
  let state = defaultState
  if (data) {
    try {
      state = JSON.parse(data)
    } catch (e) {
      console.error('Could not parse local storage data: ', e)
    }
  }
  return cleanData(state)
}

export const debouncedSave = debounce<NewTab.State>((data: NewTab.State) => {
  if (data) {
    window.localStorage.setItem(keyName, JSON.stringify(cleanData(data)))
  }
}, 50)
