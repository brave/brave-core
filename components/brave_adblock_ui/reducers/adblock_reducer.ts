/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'

const { types } = require('../constants/adblock_types')
const storage = require('../storage')

const adblockReducer: Reducer<AdBlock.State> = (state: AdBlock.State, action) => {
  if (state == null) {
    state = storage.load()
  }

  const startingState = state
  switch (action.type) {
    case types.ADBLOCK_STATS_UPDATED:
      state = storage.getLoadTimeData(state)
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
