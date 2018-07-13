/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
/* global chrome */

const types = require('../constants/adblock_types')
const storage = require('../storage')
let getGridSites_

const adblockReducer = (state, action) => {
  if (state === undefined) {
    state = storage.load() || {}
    state = Object.assign(storage.getInitialState(), state)
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
