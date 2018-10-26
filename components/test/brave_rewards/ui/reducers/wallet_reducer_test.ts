/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
/* global chrome */

import reducers from '../../../../brave_rewards/resources/ui/reducers/index'
import * as actions from '../../../../brave_rewards/resources/ui/actions/rewards_actions'
import { types } from '../../../../brave_rewards/resources/ui/constants/rewards_types'
import { defaultState } from '../../../../brave_rewards/resources/ui/storage'

describe('wallet reducer', () => {
  const constantDate = new Date('2018-01-01T12:00:00')

  beforeAll(() => {
    global.Date = class extends Date {
      constructor () {
        super()
        return constantDate
      }
    }
  });

  it('should handle initial state', () => {
    const assertion = reducers(undefined, actions.createWallet())
    expect(assertion).toEqual({
      rewardsData: defaultState
    })
  })

  describe('CREATE_WALLET_REQUESTED', () => {
    it('calls createWalletRequested', () => {
      // TODO: mock chrome.send and use jest.spyOn()
    })
  })

  describe('ADD_FUNDS_TO_WALLET', () => {
    it('calls addFundsToWallet', () => {
      // TODO: mock chrome.send and use jest.spyOn()
    })
  })

  describe('WALLET_CREATED', () => {
    it('wallet created', () => {
      const assertion = reducers(undefined, {
        type: types.WALLET_CREATED,
        payload: {
          walletCreateFailed: false,
          walletCreated: true
        }
      })

      const expectedState: Rewards.State = { ...defaultState }
      expectedState.walletCreated = true
      expectedState.enabledMain = true
      expectedState.createdTimestamp = constantDate.getTime()

      expect(assertion).toEqual({
        rewardsData: expectedState
      })
    })
  })

  describe('WALLET_CREATE_FAILED', () => {
    it('wallet failed', () => {
      const assertion = reducers(undefined, {
        type: types.WALLET_CREATE_FAILED,
        payload: {
          walletCreateFailed: true,
          walletCreated: false
        }
      })

      const expectedState: Rewards.State = { ...defaultState }
      expectedState.walletCreateFailed = true


      expect(assertion).toEqual({
        rewardsData: expectedState
      })
    })
  })
})
