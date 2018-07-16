/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'

// Constants
import { types } from '../constants/welcome_types'

// Utils
import * as storage from '../storage'

const welcomeReducer: Reducer<Welcome.State | undefined> = (state: Welcome.State | undefined, action: any) => {
  if (state === undefined) {
    state = storage.load()
  }

  const payload = action.payload
  const startingState = state
  switch (action.type) {
    case types.IMPORT_NOW_REQUESTED:
      chrome.send('importNowRequested', [])
      break
    case types.GO_TO_PAGE_REQUESTED:
      state = { ...state }
      state.pageIndex = payload.pageIndex
      break
  }

  if (state !== startingState) {
    storage.debouncedSave(state)
  }

  return state
}

export default welcomeReducer
