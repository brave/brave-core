/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'
import { types } from './constants'

function performSideEffect (fn: () => void): void {
  window.setTimeout(() => fn(), 0)
}

const ftxReducer: Reducer<NewTab.State | undefined> = (state: NewTab.State, action) => {
  const payload = action.payload
 
  switch (action.type) { 
    case types.ON_INTERACTION:
      performSideEffect(async function () {
        // TODO: chrome call
      })
      break

    case types.ON_MARKETS_OPT_IN:
      state = { ...state }
      state.ftxState.optedIntoMarkets = payload.show
      break

    default:
      break
  }

  return state
}

export default ftxReducer
