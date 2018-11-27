/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Constants
import { images as backgrounds } from './constants/backgrounds'

// Utils
import { debounce } from '../common/debounce'

const keyName = 'new-tab-data'

const defaultState: NewTab.State = {
  topSites: [],
  ignoredTopSites: [],
  pinnedTopSites: [],
  gridSites: [],
  showImages: true,
  imageLoadFailed: false,
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

const randomBackgroundImage = (): NewTab.Image => {
  const randomIndex: number = Math.floor(Math.random() * backgrounds.length)
  const image: NewTab.Image = Object.assign({}, backgrounds[randomIndex])
  image.style = {
    backgroundImage: 'url(' + image.source + ')'
  }
  return image
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
  state.backgroundImage = randomBackgroundImage()
  delete state.imageLoadFailed
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
