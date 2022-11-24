/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { createReducer } from '../../../../brave_rewards/resources/page/reducers'
import { types } from '../../../../brave_rewards/resources/page/actions/rewards_types'
import { defaultState } from '../../../../brave_rewards/resources/page/reducers/default_state'

describe('rewards reducer', () => {
  const reducers = createReducer()
  const constantDate = new Date('2018-01-01T12:00:00')

  beforeAll(() => {
    (global as any).Date = class extends Date {
      constructor () {
        super()
        return constantDate
      }
    }
  })

  describe('ON_AUTO_CONTRIBUTE_PROPERTIES', () => {
    describe('empty wallet', () => {
      it('import flow - empty', () => {
        const expectedState: Rewards.State = defaultState()

        const assertion = reducers(undefined, {
          type: types.ON_AUTO_CONTRIBUTE_PROPERTIES,
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
        const expectedState: Rewards.State = defaultState()

        const assertion = reducers(undefined, {
          type: types.ON_AUTO_CONTRIBUTE_PROPERTIES,
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
        const initState: Rewards.State = defaultState()

        const expectedState: Rewards.State = defaultState()

        const assertion = reducers({
          rewardsData: initState
        }, {
          type: types.ON_AUTO_CONTRIBUTE_PROPERTIES,
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

  describe('ON_ADS_DATA', () => {
    describe('updates ads data', () => {
      it('updates existing properties', () => {
        const initState: Rewards.State = defaultState()
        initState.adsData = {
          adsEnabled: false,
          adsPerHour: 2,
          adsSubdivisionTargeting: 'US-CA',
          automaticallyDetectedAdsSubdivisionTargeting: 'US-FL',
          shouldAllowAdsSubdivisionTargeting: true,
          subdivisions: [],
          isAdsSubdivisionTargetingRegion: true,
          adsUIEnabled: false,
          adsIsSupported: false,
          adsEstimatedPendingRewards: 0,
          adsNextPaymentDate: 0,
          adsReceivedThisMonth: 0
        }

        const expectedState: Rewards.State = defaultState()
        expectedState.adsData = {
          adsEnabled: true,
          adsPerHour: 5,
          adsSubdivisionTargeting: 'US-CA',
          automaticallyDetectedAdsSubdivisionTargeting: 'US-FL',
          shouldAllowAdsSubdivisionTargeting: true,
          subdivisions: [],
          isAdsSubdivisionTargetingRegion: true,
          adsUIEnabled: true,
          adsIsSupported: true,
          adsEstimatedPendingRewards: 0,
          adsNextPaymentDate: 0,
          adsReceivedThisMonth: 0
        }

        const assertion = reducers({
          rewardsData: initState
        }, {
          type: types.ON_ADS_DATA,
          payload: {
            adsData: {
              adsEnabled: true,
              adsPerHour: 5,
              adsSubdivisionTargeting: 'US-CA',
              automaticallyDetectedAdsSubdivisionTargeting: 'US-FL',
              shouldAllowAdsSubdivisionTargeting: true,
              subdivisions: [],
              adsUIEnabled: true,
              adsIsSupported: true
            }
          }
        })
        expect(assertion).toEqual({
          rewardsData: expectedState
        })
      })
    })
  })

  describe('ON_INLINE_TIP_SETTINGS_CHANGE', () => {
    it('state does not have inlineTip', () => {
      const expectedState: Rewards.State = {
        ...defaultState(),
        inlineTip: {
          twitter: true,
          reddit: true,
          github: true
        }
      }

      const assertion = reducers({
        rewardsData: defaultState()
      }, {
        type: types.ON_INLINE_TIP_SETTINGS_CHANGE,
        payload: {
          key: 'twitter',
          value: true
        }
      })
      expect(assertion).toEqual({
        rewardsData: expectedState
      })
    })

    it('value is empty', () => {
      const initState: Rewards.State = defaultState()

      const expectedState: Rewards.State = defaultState()
      expectedState.inlineTip = {
        twitter: true,
        reddit: true,
        github: true
      }

      const assertion = reducers({
        rewardsData: initState
      }, {
        type: types.ON_INLINE_TIP_SETTINGS_CHANGE,
        payload: {
          key: 'twitter',
          value: null
        }
      })
      expect(assertion).toEqual({
        rewardsData: expectedState
      })
    })

    it('key is empty', () => {
      const initState: Rewards.State = defaultState()

      const expectedState: Rewards.State = defaultState()
      expectedState.inlineTip = {
        twitter: true,
        reddit: true,
        github: true
      }

      const assertion = reducers({
        rewardsData: initState
      }, {
        type: types.ON_INLINE_TIP_SETTINGS_CHANGE,
        payload: {
          key: '',
          value: true
        }
      })
      expect(assertion).toEqual({
        rewardsData: expectedState
      })
    })

    it('all ok for twitter', () => {
      const initState: Rewards.State = defaultState()

      const expectedState: Rewards.State = defaultState()
      expectedState.inlineTip = {
        twitter: false,
        reddit: true,
        github: true
      }

      const assertion = reducers({
        rewardsData: initState
      }, {
        type: types.ON_INLINE_TIP_SETTINGS_CHANGE,
        payload: {
          key: 'twitter',
          value: false
        }
      })
      expect(assertion).toEqual({
        rewardsData: expectedState
      })
    })

    it('all ok for reddit', () => {
      const initState: Rewards.State = defaultState()

      const expectedState: Rewards.State = defaultState()
      expectedState.inlineTip = {
        twitter: true,
        reddit: false,
        github: true
      }

      const assertion = reducers({
        rewardsData: initState
      }, {
        type: types.ON_INLINE_TIP_SETTINGS_CHANGE,
        payload: {
          key: 'reddit',
          value: false
        }
      })
      expect(assertion).toEqual({
        rewardsData: expectedState
      })
    })

    it('all ok for github', () => {
      const initState: Rewards.State = defaultState()

      const expectedState: Rewards.State = defaultState()
      expectedState.inlineTip = {
        twitter: true,
        reddit: true,
        github: false
      }

      const assertion = reducers({
        rewardsData: initState
      }, {
        type: types.ON_INLINE_TIP_SETTINGS_CHANGE,
        payload: {
          key: 'github',
          value: false
        }
      })
      expect(assertion).toEqual({
        rewardsData: expectedState
      })
    })
  })
})
