/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
/* global window */

import { Reducer } from 'redux'

// Utils
import * as storage from '../storage'

const walletReducer: Reducer<Wallet.State | undefined> = (state: Wallet.State | undefined, action: any) => {
  if (state === undefined) {
    state = storage.load()
  }

  return state
}

export default walletReducer
