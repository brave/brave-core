/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'
import { types } from '../constants/bitcoin_dot_com_types'

const bitcoinDotComReducer: Reducer<NewTab.State | undefined> = (state: NewTab.State, action) => {
  switch (action.type) {
    case types.BUY_BITCOIN_DOT_COM_CRYPTO:
      chrome.moonpay.onBuyBitcoinDotComCrypto()
      break

    case types.INTERACTION_BITCOIN_DOT_COM:
      chrome.moonpay.onInteractionBitcoinDotCom()
      break

    default:
      break
  }

  return state
}

export default bitcoinDotComReducer
