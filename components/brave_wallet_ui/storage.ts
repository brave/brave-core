/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Utils
import { debounce } from '../common/debounce'

export const defaultState = {}

const keyName = 'wallet-data'
const cleanData = (state: Wallet.State) => {
  state = { ...state }
  return state
}

export const load = (): Wallet.State => {
  const data = window.localStorage.getItem(keyName)
  let state = defaultState
  if (data) {
    try {
      state = JSON.parse(data)
    } catch (e) {
      console.error('Could not parse local storage data for Brave Wallet: ', e)
    }
  }
  return cleanData(state)
}

export const debouncedSave = debounce((data: Wallet.State) => {
  if (data) {
    window.localStorage.setItem(keyName, JSON.stringify(cleanData(data)))
  }
}, 50)
