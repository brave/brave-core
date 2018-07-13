/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/* global chrome */

const debounce = require('../common/debounce')

const keyName = 'adblock-data'

const cleanData = (state) => {
  return module.exports.getLoadTimeData(state)
}

module.exports.getLoadTimeData = (state) => {
  state = { ...state }
  state.stats = {}
  // Expected to be numbers
  ;['adsBlockedStat', 'regionalAdBlockEnabled'].forEach(
      (stat) => { state.stats[stat] = parseInt(chrome.getVariableValue(stat)) })
  // Expected to be Strings
  ;['regionalAdBlockTitle'].forEach(
      (stat) => { state.stats[stat] = chrome.getVariableValue(stat) })
  return state
}

module.exports.getInitialState = () => cleanData({
  stats: {
    numBlocked: 0,
    regionalAdBlockEnabled: false
  }
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
