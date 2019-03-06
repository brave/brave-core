/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'

// Constants
import { types } from '../constants/adblock_types'

// Utils
import * as storage from '../storage'
import { debounce } from '../../common/debounce'

const updateCustomFilters = debounce((customFilters: string) => {
  chrome.send('brave_adblock.updateCustomFilters', [customFilters])
}, 1500)

const adblockReducer: Reducer<AdBlock.State | undefined> = (state: AdBlock.State | undefined, action) => {
  if (state === undefined) {
    state = storage.load()
  }

  const startingState = state
  switch (action.type) {
    case types.ADBLOCK_GET_CUSTOM_FILTERS:
      chrome.send('brave_adblock.getCustomFilters')
      break
    case types.ADBLOCK_ON_GET_CUSTOM_FILTERS:
      state = { ...state, settings: { customFilters: action.payload.customFilters } }
      break
    case types.ADBLOCK_STATS_UPDATED:
      state = storage.getLoadTimeData(state)
      break
    case types.ADBLOCK_UPDATE_CUSTOM_FILTERS:
      state = { ...state, settings: { customFilters: action.payload.customFilters } }
      updateCustomFilters(state.settings.customFilters)
      break
    default:
      break
  }

  if (state !== startingState) {
    storage.debouncedSave(state)
  }

  return state
}

export default adblockReducer
