/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
/* global chrome */

import rewardsReducer from '../../../brave_rewards_ui/reducers/rewards_reducer'
import * as actions from '../../../brave_rewards_ui/actions/rewards_actions'
import { types } from '../../../brave_rewards_ui/constants/rewards_types'

describe('rewardsReducer', () => {
  it('should handle initial state', () => {
    const assertion = rewardsReducer(undefined, actions.walletCreated())
    expect(assertion).toEqual({
      walletCreateFailed: false,
      walletCreated: true
    })
  })

  describe('CREATE_WALLET_REQUESTED', () => {
    it('calls createWalletRequested', () => {
      // TODO: mock chrome.send and use jest.spyOn()
    })
  })

  describe('WALLET_CREATED', () => {
    it('wallet created', () => {
      const assertion = rewardsReducer(undefined, {
        type: types.WALLET_CREATED,
        payload: {
          walletCreateFailed: false,
          walletCreated: true
        }
      })
      expect(assertion).toEqual({
        walletCreateFailed: false,
        walletCreated: true
      })
    })
  })

  describe('WALLET_CREATE_FAILED', () => {
    it('wallet failed', () => {
      const assertion = rewardsReducer(undefined, {
        type: types.WALLET_CREATE_FAILED,
        payload: {
          walletCreateFailed: true,
          walletCreated: false
        }
      })
      expect(assertion).toEqual({
        walletCreateFailed: true,
        walletCreated: false
      })
    })
  })
})
