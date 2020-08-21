/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'
import { types } from '../constants/cryptoDotCom_types'

const cryptoDotComReducer: Reducer<NewTab.State | undefined> = (state: NewTab.State, action) => {
  switch (action.type) {
    case types.ON_TOTAL_PRICE_OPT_IN:
      state = { ...state }
      state.cryptoDotComState.optInTotal = true
      break

    case types.ON_BTC_PRICE_OPT_IN:
      state = { ...state }
      state.cryptoDotComState.optInBTCPrice = true
      break

    default:
      break
  }

  return state
}

export default cryptoDotComReducer
