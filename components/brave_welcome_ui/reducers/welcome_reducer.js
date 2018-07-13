/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/* global chrome */

const types = require('../constants/welcome_types')
const storage = require('../storage')

const welcomeReducer = (state, action) => {
  if (state === undefined) {
    state = storage.load() || {}
    state = Object.assign(storage.getInitialState(), state)
  }
  const startingState = state
  switch (action.type) {
    case types.IMPORT_NOW_REQUESTED:
      chrome.send('importNowRequested', [])
      break
    case types.GO_TO_PAGE_REQUESTED:
      state = {...state}
      state.pageIndex = action.pageIndex
      break
  }

  if (state !== startingState) {
    storage.debouncedSave(state)
  }

  return state
}

export default welcomeReducer
