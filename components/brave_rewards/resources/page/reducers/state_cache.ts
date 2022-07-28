/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { debounce } from '../../../../common/debounce'
import { defaultState } from './default_state'

const keyName = 'rewards-data'

export const loadState = (): Rewards.State => {
  const data = window.localStorage.getItem(keyName)
  if (!data) {
    return defaultState
  }

  let parsedData: any
  try {
    parsedData = JSON.parse(data)
  } catch {
    parsedData = null
  }

  if (!parsedData || typeof parsedData !== 'object') {
    console.error('Local storage data is not an object')
    return defaultState
  }

  if (parsedData.version !== defaultState.version) {
    console.error('Local storage state version does not match')
    return defaultState
  }

  parsedData.initializing = true
  return parsedData as Rewards.State
}

export const saveState = debounce((data: Rewards.State) => {
  if (data) {
    window.localStorage.setItem(keyName, JSON.stringify(data))
  }
}, 150)
