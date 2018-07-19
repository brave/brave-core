/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { types } from '../../../brave_rewards_ui/constants/rewards_types'
import * as actions from '../../../brave_rewards_ui/actions/rewards_actions'

describe('rewards_actions', () => {
  it('createWalletRequested', () => {
    expect(actions.createWalletRequested()).toEqual({
      type: types.CREATE_WALLET_REQUESTED,
      meta: undefined,
      payload: undefined
    })
  })

  it('walletCreated', () => {
    expect(actions.walletCreated()).toEqual({
      type: types.WALLET_CREATED,
      meta: undefined,
      payload: undefined
    })
  })

  it('walletCreateFailed', () => {
    expect(actions.walletCreateFailed()).toEqual({
      type: types.WALLET_CREATE_FAILED,
      meta: undefined,
      payload: undefined
    })
  })
})
