/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { debounce } from '../common/debounce'

const keyName = 'adblock-data'

export const defaultState: AdBlock.State = {
  settings: {
    customFilters: '',
    regionalLists: [],
    listSubscriptions: []
  }
}

export const load = (): AdBlock.State => {
  const data = window.localStorage.getItem(keyName)
  let state: AdBlock.State = defaultState
  if (data) {
    try {
      state = JSON.parse(data)
    } catch (e) {
      console.error('Could not parse local storage data: ', e)
    }
  }
  return state
}

export const debouncedSave = debounce((data: AdBlock.State) => {
  if (data) {
    window.localStorage.setItem(keyName, JSON.stringify(data))
  }
}, 50)
