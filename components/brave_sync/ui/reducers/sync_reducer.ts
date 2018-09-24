/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'

// Constants
import { types } from '../constants/sync_types'

// Utils
import * as storage from '../storage'

const syncReducer: Reducer<Sync.State | undefined> = (state: Sync.State | undefined, action: any) => {
  if (state === undefined) {
    state = storage.load()
  }

  const payload = action.payload
  const startingState = state
  switch (action.type) {
    case types.TEST_SYNC_ACTION:
      state = { ...state }
      state.something = payload.something
      console.log('reducer received ', state.something)
      break
    // case types.types.APP_SET_STATE:
      // chrome.send('', [])
      // break
    // case types.APP_CREATE_SYNC_CACHE:
    // case types.APP_PENDING_SYNC_RECORDS_ADDED:
    // case types.APP_PENDING_SYNC_RECORDS_REMOVED:
    // case types.APP_SAVE_SYNC_DEVICES:
    // case types.APP_SAVE_SYNC_INIT_DATA:
    // case types.APP_RESET_SYNC_DATA:
    // case types.APP_SET_SYNC_SETUP_ERROR:
    // case types.APP_PENDING_SYNC_RECORDS_ADDED
    // case types.APP_PENDING_SYNC_RECORDS_REMOVED
  }

  if (state !== startingState) {
    storage.debouncedSave(state)
  }

  return state
}

export default syncReducer
