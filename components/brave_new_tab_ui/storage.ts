/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/* global chrome */

const debounce = require('../common/debounce')
const backgrounds = require('./backgrounds')

const keyName = 'new-tab-data'

const randomBackgroundImage = () => {
  const randomIndex = Math.floor(Math.random() * backgrounds.length)
  const image = Object.assign({}, backgrounds[randomIndex])
  image.style = {backgroundImage: 'url(' + image.source + ')'}
  return image
}

const cleanData = (state) => {
  state = { ...state }
  state.backgroundImage = randomBackgroundImage()
  delete state.imageLoadFailed
  state = module.exports.getLoadTimeData(state)
  return state
}

module.exports.getLoadTimeData = (state) => {
  state = { ...state }
  state.stats = {}
  ;['adsBlockedStat', 'trackersBlockedStat', 'javascriptBlockedStat',
    'httpsUpgradesStat', 'fingerprintingBlockedStat'].forEach(
      (stat) => { state.stats[stat] = parseInt(chrome.getVariableValue(stat)) })
  return state
}

module.exports.getInitialState = () => cleanData({
  topSites: [],
  ignoredTopSites: [],
  pinnedTopSites: [],
  gridSites: [],
  showImages: true,
  imageLoadFailed: false,
  showEmptyPage: false,
  isIncognito: chrome.extension.inIncognitoContext,
  useAlternativePrivateSearchEngine: false,
  bookmarks: {},
  stats: {}
})

module.exports.load = () => {
  const data = window.localStorage.getItem(keyName)
  let state
  if (data) {
    try {
      state = JSON.parse(data)
    } catch (e) {
      console.error('Could not parse local storage data: ', e)
    }
  }
  return cleanData(state)
}

module.exports.debouncedSave = debounce((data) => {
  if (data) {
    window.localStorage.setItem(keyName, JSON.stringify(cleanData(data)))
  }
}, 50)
