/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Utils
import { debounce } from '../common/debounce'

const keyName = 'welcome-data'

export const defaultState = {
  searchProviders: [],
  browserProfiles: [],
  hideSearchOnboarding: false
}

const cleanData = (state: Welcome.State) => {
  state = { ...state }
  return state
}

export const load = (): Welcome.State => {
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

export const debouncedSave = debounce((data: Welcome.State) => {
  if (data) {
    window.localStorage.setItem(keyName, JSON.stringify(cleanData(data)))
  }
}, 50)
