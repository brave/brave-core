/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
/* global chrome */

import reducers from '../../../../brave_rewards/resources/page/reducers/index'
import { types } from '../../../../brave_rewards/resources/page/constants/rewards_types'
import { defaultState } from '../../../../brave_rewards/resources/page/storage'

describe('Promotion Reducer', () => {

  describe('FETCH_PROMOTIONS', () => {
    it('does not modify state', () => {
      const assertion = reducers(undefined, {
        type: types.FETCH_PROMOTIONS,
        payload: {}
      })

      const expectedState: Rewards.State = { ...defaultState }

      expect(assertion).toEqual({
        rewardsData: expectedState
      })
    })
  })

  describe('ON_PROMOTION', () => {
    it('pushes new promotion to state', () => {
      const assertion = reducers(undefined, {
        type: types.ON_PROMOTIONS,
        payload: {
          properties: {
            result: 0,
            promotions: [
              {
                promotionId: 'test-promotion-id',
                status: 0,
                type: 0,
                expiresAt: 140
              }
            ]
          }
        }
      })

      const promotions = [
        {
          promotionId: 'test-promotion-id',
          status: 0,
          type: 0,
          expiresAt: 140000
        }
      ]
      const expectedState: Rewards.State = { ...defaultState, promotions }

      expect(assertion).toEqual({
        rewardsData: expectedState
      })
    })

    it('does not modify state on status 1', () => {
      const assertion = reducers(undefined, {
        type: types.ON_PROMOTIONS,
        payload: {
          properties: {
            status: 1,
            promotions: []
          }
        }
      })

      const expectedState: Rewards.State = { ...defaultState }

      expect(assertion).toEqual({
        rewardsData: expectedState
      })
    })

    it('clears promotions if no promotions', () => {
      const initialState = {
        ...defaultState
      }
      initialState.promotions = [
        {
          promotionId: 'test-promotion-id',
          status: 0,
          type: 0,
          expiresAt: 140000
        },
        {
          promotionId: 'test-promotion-id-2',
          status: 0,
          type: 1,
          expiresAt: 140000
        }
      ]

      const assertion = reducers({
        rewardsData: initialState
      }, {
        type: types.ON_PROMOTIONS,
        payload: {
          properties: {
            promotions: [],
            status: 0
          }
        }
      })

      const expectedState: Rewards.State = { ...defaultState }

      expect(assertion).toEqual({
        rewardsData: expectedState
      })
    })
  })

  describe('ON_CLAIM_PROMOTION', () => {
    it('does not modify state if no promotions or current promotion', () => {
      const assertion = reducers({
        rewardsData: defaultState
      }, {
        type: types.ON_CLAIM_PROMOTION,
        payload: {
          properties: {
            promotionId: 'test-id',
            captchaImage: 'XXX',
            captchaId: 'id',
            hint: 'circle'
          }
        }
      })

      const expectedState: Rewards.State = { ...defaultState }

      expect(assertion).toEqual({
        rewardsData: expectedState
      })
    })

    it('modifies correct promotion with captcha information', () => {
      const initialState = {
        ...defaultState
      }
      initialState.promotions = [
        {
          promotionId: 'test-promotion-id',
          status: 0,
          type: 0,
          expiresAt: 140000
        },
        {
          promotionId: 'test-promotion-id-2',
          status: 0,
          type: 1,
          expiresAt: 140000
        }
      ]

      const assertion = reducers({
        rewardsData: initialState
      }, {
        type: types.ON_CLAIM_PROMOTION,
        payload: {
          properties: {
            promotionId: 'test-promotion-id-2',
            captchaImage: 'XXX',
            captchaId: 'id',
            hint: 'circle'
          }
        }
      })

      const expectedState: Rewards.State = { ...defaultState }
      expectedState.promotions = [
        {
          promotionId: 'test-promotion-id',
          status: 0,
          type: 0,
          expiresAt: 140000
        },
        {
          promotionId: 'test-promotion-id-2',
          status: 0,
          type: 1,
          expiresAt: 140000,
          captchaImage: 'XXX',
          captchaId: 'id',
          hint: 'circle'
        }
      ]

      expect(assertion).toEqual({
        rewardsData: expectedState
      })
    })
  })

  describe('RESET_PROMOTION', () => {
    it('resets current promotion', () => {
      const initialState = {
        ...defaultState
      }
      initialState.promotions = [
        {
          promotionId: 'test-promotion-id',
          expiresAt: 0,
          amount: 1.0,
          type: 0
        },
        {
          promotionId: 'test-promotion-id-2',
          expiresAt: 0,
          amount: 1.0,
          type: 1,
          captchaImage: 'XXX',
          hint: 'blue'
        }
      ]

      const assertion = reducers({
        rewardsData: initialState
      }, {
        type: types.RESET_PROMOTION,
        payload: {
          promotionId: 'test-promotion-id-2'
        }
      })

      const expectedState: Rewards.State = { ...defaultState }
      expectedState.promotions = [
        {
          promotionId: 'test-promotion-id',
          expiresAt: 0,
          amount: 1.0,
          type: 0
        },
        {
          promotionId: 'test-promotion-id-2',
          amount: 1,
          captchaId: '',
          captchaImage: '',
          captchaStatus: null,
          expiresAt: 0,
          hint: '',
          type: 1
        }
      ]

      expect(assertion).toEqual({
        rewardsData: expectedState
      })
    })
  })

  describe('DELETE_PROMOTION', () => {
    it('deletes promotion', () => {
      const initialState = {
        ...defaultState
      }
      initialState.promotions = [
        {
          promotionId: 'test-promotion-id',
          expiresAt: 0,
          amount: 1.0,
          type: 0
        },
        {
          promotionId: 'test-promotion-id-2',
          expiresAt: 0,
          amount: 1.0,
          type: 1,
          captchaImage: 'XXX',
          hint: 'blue'
        }
      ]

      const assertion = reducers({
        rewardsData: initialState
      }, {
        type: types.DELETE_PROMOTION,
        payload: {
          promotionId: 'test-promotion-id-2'
        }
      })

      const expectedState: Rewards.State = { ...defaultState }
      expectedState.promotions = [
        {
          promotionId: 'test-promotion-id',
          expiresAt: 0,
          amount: 1.0,
          type: 0
        },
        {
          promotionId: 'test-promotion-id-2',
          amount: 1,
          captchaImage: 'XXX',
          captchaStatus: null,
          expiresAt: 0,
          hint: 'blue',
          status: 4,
          type: 1
        }
      ]

      expect(assertion).toEqual({
        rewardsData: expectedState
      })
    })
  })

  describe('ON_PROMOTION_FINISH', () => {
    it('does not modify state if there are no promotions available', () => {
      const assertion = reducers(undefined, {
        type: types.ON_PROMOTION_FINISH,
        payload: {
          properties: {
            result: 0,
            promotion: {
              promotionId: 'test-promotion-id',
              expiresAt: 11000,
              amount: 30.0
            }
          }
        }
      })

      const expectedState: Rewards.State = {
        ...defaultState,
        promotions: [],
        firstLoad: false
      }

      expect(assertion).toEqual({
        rewardsData: expectedState
      })
    })
    it('modifies correct promotion with updated status info (0)', () => {
      const initialState = {
        ...defaultState
      }
      initialState.promotions = [
        {
          promotionId: 'test-promotion-id',
          captchaStatus: 'start',
          expiresAt: 0,
          amount: 1.0,
          type: 0
        },
        {
          promotionId: 'test-promotion-id-2',
          expiresAt: 0,
          amount: 1.0,
          type: 1,
          captchaImage: 'XXX',
          hint: 'blue'
        }
      ]
      const assertion = reducers({
        rewardsData: initialState
      }, {
        type: types.ON_PROMOTION_FINISH,
        payload: {
          properties: {
            result: 0,
            promotion: {
              promotionId: 'test-promotion-id',
              expiresAt: 0,
              amount: 1.0,
              type: 1
            }
          }
        }
      })

      const expectedState: Rewards.State = { ...defaultState }
      expectedState.promotions = [
        {
          promotionId: 'test-promotion-id',
          captchaId: '',
          captchaImage: '',
          captchaStatus: 'start',
          expiresAt: 0,
          hint: '',
          amount: 1.0,
          type: 0,
          status: 4
        },
        {
          promotionId: 'test-promotion-id-2',
          expiresAt: 0,
          amount: 1.0,
          type: 1,
          captchaImage: 'XXX',
          hint: 'blue'
        }
      ]
      expectedState.ui.emptyWallet = false

      expect(assertion).toEqual({
        rewardsData: expectedState
      })
    })
    it('modifies correct promotion with updated status info (6)', () => {
      const initialState = {
        ...defaultState
      }
      initialState.promotions = [
        {
          promotionId: 'test-promotion-id',
          expiresAt: 0,
          amount: 1.0,
          type: 0
        },
        {
          promotionId: 'test-promotion-id-2',
          expiresAt: 0,
          amount: 1.0,
          type: 1,
          captchaImage: 'XXX',
          hint: 'blue'
        }
      ]

      const assertion = reducers({
        rewardsData: initialState
      }, {
        type: types.ON_PROMOTION_FINISH,
        payload: {
          properties: {
            result: 6,
            promotion: {
              promotionId: 'test-promotion-id'
            }
          }
        }
      })

      const expectedState: Rewards.State = { ...defaultState }
      expectedState.promotions = [
        {
          promotionId: 'test-promotion-id',
          captchaStatus: 'wrongPosition',
          expiresAt: 0,
          amount: 1.0,
          type: 0
        },
        {
          promotionId: 'test-promotion-id-2',
          expiresAt: 0,
          amount: 1.0,
          type: 1,
          captchaImage: 'XXX',
          hint: 'blue'
        }
      ]
      expectedState.ui.emptyWallet = false

      expect(assertion).toEqual({
        rewardsData: expectedState
      })
    })
  })
})
