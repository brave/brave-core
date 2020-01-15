/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { debounce } from '../common/debounce'

const keyName = 'playlists-data'

export const defaultState: Playlists.State = {
  settings: {
    customFilters: '',
    regionalLists: []
  },
  stats: {
    numBlocked: 0
  }
}

export const getLoadTimeData = (state: Playlists.State): Playlists.State => {
  state = { ...state }
  state.stats = defaultState.stats

  // Expected to be numbers
  ;['adsBlockedStat'].forEach((stat) => {
    state.stats[stat] = parseInt(chrome.getVariableValue(stat), 10)
  })

  return state
}

export const cleanData = (state: Playlists.State): Playlists.State => {
  return getLoadTimeData(state)
}

export const load = (): Playlists.State => {
  const data = window.localStorage.getItem(keyName)
  let state: Playlists.State = defaultState
  if (data) {
    try {
      state = JSON.parse(data)
    } catch (e) {
      console.error('Could not parse local storage data: ', e)
    }
  }
  return cleanData(state)
}

export const debouncedSave = debounce((data: Playlists.State) => {
  if (data) {
    window.localStorage.setItem(keyName, JSON.stringify(cleanData(data)))
  }
}, 50)
