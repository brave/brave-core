/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const debounce = require('../common/debounce')

const keyName = 'rewards-data'

const defaultState: Rewards.State = {
  walletCreated: false,
  walletCreateFailed: false
}

export const getInitialState = (): Rewards.State => cleanData(defaultState)

export const load = () => {
  const data = window.localStorage.getItem(keyName)
  let state: Rewards.State = getInitialState()
  if (data) {
    try {
      state = JSON.parse(data)
    } catch (e) {
      console.error('Could not parse local storage data: ', e)
    }
  }
  return cleanData(state)
}

export const debouncedSave = debounce((data: Rewards.State) => {
  if (data) {
    window.localStorage.setItem(keyName, JSON.stringify(cleanData(data)))
  }
}, 50)
