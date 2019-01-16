/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
/* global chrome */

import reducers from '../../../../../../brave_rewards/resources/extension/brave_rewards/background/reducers'
import { types } from '../../../../../../brave_rewards/resources/extension/brave_rewards/constants/rewards_panel_types'
import { defaultState } from '../../../../../../brave_rewards/resources/extension/brave_rewards/background/storage'

describe('rewards panel reducer', () => {
  const constantDate = new Date('2018-01-01T12:00:00')

  beforeAll(() => {
    (global as any).Date = class extends Date {
      constructor () {
        super()
        return constantDate
      }
    }
  })

  describe('ON_TAB_RETRIEVED', () => {
    describe('persist publisher info', () => {
      it('url is the same', () => {
        const initState: Rewards.State = { ...defaultState, walletCreated: true }
        const payload = {
          tab: {
            url: 'https://clifton.io',
            incognito: false,
            active: true,
            windowId: 1
          }
        }

        // first visit
        const expectedState1: Rewards.State = {
          ...defaultState,
          walletCreated: true,
          publishers: {
            id_1: {
              tabUrl: 'https://clifton.io'
            }
          }
        }

        let state = reducers({ rewardsPanelData: initState }, {
          type: types.ON_TAB_RETRIEVED,
          payload
        })

        expect(state.rewardsPanelData).toEqual(expectedState1)

        // imitates ON_PUBLISHER_DATA
        state.rewardsPanelData.publishers = {
          id_1: {
            tabUrl: 'https://clifton.io',
            name: 'Clifton'
          }
        }

        // second visit
        const expectedState2: Rewards.State = {
          ...defaultState,
          walletCreated: true,
          publishers: {
            id_1: {
              tabUrl: 'https://clifton.io',
              name: 'Clifton'
            }
          }
        }

        state = reducers(state, {
          type: types.ON_TAB_RETRIEVED,
          payload
        })

        expect(state.rewardsPanelData).toEqual(expectedState2)
      })

      it('url is not the same', () => {
        const initState: Rewards.State = { ...defaultState, walletCreated: true }

        // first visit
        const expectedState1: Rewards.State = {
          ...defaultState,
          walletCreated: true,
          publishers: {
            id_1: {
              tabUrl: 'https://clifton.io'
            }
          }
        }

        let state = reducers({ rewardsPanelData: initState }, {
          type: types.ON_TAB_RETRIEVED,
          payload: {
            tab: {
              url: 'https://clifton.io',
              incognito: false,
              active: true,
              windowId: 1
            }
          }
        })

        expect(state.rewardsPanelData).toEqual(expectedState1)

        // imitates ON_PUBLISHER_DATA
        state.rewardsPanelData.publishers = {
          id_1: {
            tabUrl: 'clifton.io',
            name: 'Clifton'
          }
        }

        // second visit
        const expectedState2: Rewards.State = {
          ...defaultState,
          walletCreated: true,
          publishers: {
            id_1: {
              tabUrl: 'https://brave.com'
            }
          }
        }

        state = reducers(state, {
          type: types.ON_TAB_RETRIEVED,
          payload: {
            tab: {
              url: 'https://brave.com',
              incognito: false,
              active: true,
              windowId: 1
            }
          }
        })

        expect(state.rewardsPanelData).toEqual(expectedState2)
      })
    })
  })
})
