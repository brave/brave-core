/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
/* global chrome */

import reducers from '../../../../brave_rewards/resources/page/reducers/index'
import { types } from '../../../../brave_rewards/resources/page/constants/rewards_types'
import { defaultState } from '../../../../brave_rewards/resources/page/storage'
import { getMockChrome } from '../../../testData'

describe('wallet reducer', () => {
  const constantDate = new Date('2018-01-01T12:00:00')

  beforeAll(() => {
    (global as any).Date = class extends Date {
      constructor () {
        super()
        return constantDate
      }
    }
  })

  describe('ON_RECOVER_WALLET_DATA', () => {
    let chromeSpy: jest.SpyInstance

    (global as any).chrome = getMockChrome()

    beforeEach(() => {
      chromeSpy = jest.spyOn(chrome, 'send')
    })

    afterEach(() => {
      chromeSpy.mockRestore()
    })

    it('failed to recover', () => {
      const assertion = reducers({ rewardsData: { ...defaultState } }, {
        type: types.ON_RECOVER_WALLET_DATA,
        payload: {
          result: 2
        }
      })

      const expectedState: Rewards.State = { ...defaultState }

      // No chrome.send calls should be made in the event of a failure
      expect(chromeSpy).toHaveBeenCalledTimes(0)
      expect(assertion).toEqual({
        rewardsData: expectedState
      })
    })

    it('recovered successfully', () => {
      const assertion = reducers({ rewardsData: { ...defaultState } }, {
        type: types.ON_RECOVER_WALLET_DATA,
        payload: {
          result: 0
        }
      })

      const expectedState = {
        ...defaultState,
        ui: {
          ...defaultState.ui,
          emptyWallet: false,
          modalBackup: false
        }
      }

      expect(chromeSpy).toHaveBeenCalledTimes(4)
      expect(chromeSpy.mock.calls[0][0]).toEqual('brave_rewards.fetchPromotions')
      expect(chromeSpy.mock.calls[1][0]).toEqual('brave_rewards.fetchBalance')
      expect(chromeSpy.mock.calls[2][0]).toEqual('brave_rewards.getPaymentId')
      expect(chromeSpy.mock.calls[3][0]).toEqual('brave_rewards.getBalanceReport')

      expect(assertion).toEqual({
        rewardsData: expectedState
      })
    })
  })
})
