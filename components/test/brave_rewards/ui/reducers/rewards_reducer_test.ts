/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
/* global chrome */

import reducers from '../../../../brave_rewards/resources/ui/reducers'
import { types } from '../../../../brave_rewards/resources/ui/constants/rewards_types'
import { defaultState } from '../../../../brave_rewards/resources/ui/storage'

describe('rewards reducer', () => {
  const constantDate = new Date('2018-01-01T12:00:00')

  beforeAll(() => {
    (global as any).Date = class extends Date {
      constructor () {
        super()
        return constantDate
      }
    }
  })

  describe('INIT_AUTOCONTRIBUTE_SETTINGS', () => {
    describe('empty wallet', () => {
      it('import flow - empty', () => {
        const expectedState: Rewards.State = { ...defaultState }
        expectedState.ui.emptyWallet = true

        const assertion = reducers(undefined, {
          type: types.INIT_AUTOCONTRIBUTE_SETTINGS,
          payload: {
            properties: {
              ui: {
                emptyWallet: true
              }
            }
          }
        })
        expect(assertion).toEqual({
          rewardsData: expectedState
        })
      })

      it('import flow - funded', () => {
        const expectedState: Rewards.State = { ...defaultState }
        expectedState.ui.emptyWallet = false

        const assertion = reducers(undefined, {
          type: types.INIT_AUTOCONTRIBUTE_SETTINGS,
          payload: {
            properties: {
              ui: {
                emptyWallet: false
              }
            }
          }
        })

        expect(assertion).toEqual({
          rewardsData: expectedState
        })
      })

      it('import flow - existing state', () => {
        const initState: Rewards.State = { ...defaultState }
        initState.ui.emptyWallet = false
        initState.ui.walletRecoverySuccess = true

        const expectedState: Rewards.State = { ...defaultState }
        expectedState.ui.emptyWallet = false
        expectedState.ui.walletRecoverySuccess = true

        const assertion = reducers({
          rewardsData: initState
        }, {
          type: types.INIT_AUTOCONTRIBUTE_SETTINGS,
          payload: {
            properties: {
              ui: {
                emptyWallet: true
              }
            }
          }
        })

        expect(assertion).toEqual({
          rewardsData: expectedState
        })
      })
    })
  })
})
