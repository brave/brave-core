/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
/* global chrome */

import reducers from '../../../../brave_rewards/resources/ui/reducers/index'
import { types } from '../../../../brave_rewards/resources/ui/constants/rewards_types'
import { defaultState } from '../../../../brave_rewards/resources/ui/storage'
import { rewardsInitialState } from '../../../testData'

describe('Grant Reducer', () => {

  describe('GET_GRANTS', () => {
    it('does not modify state', () => {
      const assertion = reducers(undefined, {
        type: types.GET_GRANTS,
        payload: {}
      })

      const expectedState: Rewards.State = { ...defaultState }

      expect(assertion).toEqual({
        rewardsData: expectedState
      })
    })
  })

  describe('ON_GRANT', () => {
    it('pushes new grant to state', () => {
      const assertion = reducers(undefined, {
        type: types.ON_GRANT,
        payload: {
          properties: {
            promotionId: 'test-promotion-id',
            status: 0,
            type: 'ugp'
          }
        }
      })

      const grants = [
        {
          promotionId: 'test-promotion-id',
          expiryTime: 0,
          probi: '',
          type: 'ugp'
        }
      ]
      const expectedState: Rewards.State = { ...defaultState, grants }

      expect(assertion).toEqual({
        rewardsData: expectedState
      })
    })
    it('does not modify state on status 1', () => {
      const assertion = reducers(undefined, {
        type: types.ON_GRANT,
        payload: {
          properties: {
            promotionId: 'test-promotion-id',
            status: 1,
            type: 'ugp'
          }
        }
      })

      const expectedState: Rewards.State = { ...defaultState }

      expect(assertion).toEqual({
        rewardsData: expectedState
      })
    })
  })

  describe('GET_GRANT_CAPTCHA', () => {
    it('sets current grant', () => {
      const initialState = {
        ...defaultState
      }
      initialState.grants = [
        {
          promotionId: 'test-promotion-id',
          expiryTime: 0,
          probi: '',
          type: 'ugp'
        },
        {
          promotionId: 'test-promotion-id-2',
          expiryTime: 0,
          probi: '',
          type: 'ads'
        }
      ]

      const assertion = reducers({
        rewardsData: initialState
      }, {
        type: types.GET_GRANT_CAPTCHA,
        payload: {
          promotionId: 'test-promotion-id-2'
        }
      })

      const currentGrant = {
        promotionId: 'test-promotion-id-2',
        expiryTime: 0,
        probi: '',
        type: 'ads'        
      }
      const expectedState: Rewards.State = {
        ...initialState,
        currentGrant
      }

      expect(assertion).toEqual({
        rewardsData: expectedState
      })
    })
    it('does not modify state if no grants', () => {
      const assertion = reducers({
        rewardsData: defaultState
      }, {
        type: types.GET_GRANT_CAPTCHA,
        payload: {
          promotionId: 'test-non-existent'
        }
      })

      const expectedState: Rewards.State = { ...defaultState }

      expect(assertion).toEqual({
        rewardsData: expectedState
      })
    })
  })

  describe('ON_GRANT_CAPTCHA', () => {
    it('does not modify state if no grants or current grant', () => {
      const assertion = reducers({
        rewardsData: defaultState
      }, {
        type: types.ON_GRANT_CAPTCHA,
        payload: {
          captcha: {
            image: 'XXX',
            hint: 'blue'
          }
        }
      })

      const expectedState: Rewards.State = { ...defaultState }

      expect(assertion).toEqual({
        rewardsData: expectedState
      })
    })
    it('modifies correct grant with captcha information', () => {
      const initialState = {
        ...defaultState
      }
      initialState.grants = [
        {
          promotionId: 'test-promotion-id',
          expiryTime: 0,
          probi: '',
          type: 'ugp'
        },
        {
          promotionId: 'test-promotion-id-2',
          expiryTime: 0,
          probi: '',
          type: 'ads'
        }
      ]
      initialState.currentGrant = {
        promotionId: 'test-promotion-id-2',
        expiryTime: 0,
        probi: '',
        type: 'ads'
      }

      const assertion = reducers({
        rewardsData: initialState
      }, {
        type: types.ON_GRANT_CAPTCHA,
        payload: {
          captcha: {
            image: 'XXX',
            hint: 'blue'
          }
        }
      })

      const expectedState: Rewards.State = { ...defaultState }
      expectedState.currentGrant = {
        promotionId: 'test-promotion-id-2',
        expiryTime: 0,
        probi: '',
        type: 'ads'
      }
      expectedState.grants = [
        {
          promotionId: 'test-promotion-id',
          expiryTime: 0,
          probi: '',
          type: 'ugp'
        },
        {
          promotionId: 'test-promotion-id-2',
          expiryTime: 0,
          probi: '',
          type: 'ads',
          captcha: 'data:image/jpeg;base64,XXX',
          hint: 'blue'
        }
      ]

      expect(assertion).toEqual({
        rewardsData: expectedState
      })
    })
  })

  describe('ON_GRANT_RESET', () => {
    it('resets current grant', () => {
      const initialState = {
        ...defaultState
      }
      initialState.grants = [
        {
          promotionId: 'test-promotion-id',
          expiryTime: 0,
          probi: '',
          type: 'ugp'
        },
        {
          promotionId: 'test-promotion-id-2',
          expiryTime: 0,
          probi: '',
          type: 'ads',
          captcha: 'data:image/jpeg;base64,XXX',
          hint: 'blue'
        }
      ]
      initialState.currentGrant = {
        promotionId: 'test-promotion-id-2',
        expiryTime: 0,
        probi: '',
        type: 'ads'
      }

      const assertion = reducers({
        rewardsData: initialState
      }, {
        type: types.ON_GRANT_RESET,
        payload: {}
      })

      const expectedState: Rewards.State = { ...defaultState }
      expectedState.currentGrant = undefined
      expectedState.grants = [
        {
          promotionId: 'test-promotion-id',
          expiryTime: 0,
          probi: '',
          type: 'ugp'
        },
        {
          promotionId: 'test-promotion-id-2',
          expiryTime: 0,
          probi: '',
          type: 'ads'
        }
      ]

      expect(assertion).toEqual({
        rewardsData: expectedState
      })
    })
  })

  describe('ON_GRANT_DELETE', () => {
    it('deletes current grant', () => {
      const initialState = {
        ...defaultState
      }
      initialState.grants = [
        {
          promotionId: 'test-promotion-id',
          expiryTime: 0,
          probi: '',
          type: 'ugp'
        },
        {
          promotionId: 'test-promotion-id-2',
          expiryTime: 0,
          probi: '',
          type: 'ads',
          captcha: 'data:image/jpeg;base64,XXX',
          hint: 'blue'
        }
      ]
      initialState.currentGrant = {
        promotionId: 'test-promotion-id-2',
        expiryTime: 0,
        probi: '',
        type: 'ads'
      }

      const assertion = reducers({
        rewardsData: initialState
      }, {
        type: types.ON_GRANT_DELETE,
        payload: {}
      })

      const expectedState: Rewards.State = { ...defaultState }
      expectedState.currentGrant = undefined
      expectedState.grants = [
        {
          promotionId: 'test-promotion-id',
          expiryTime: 0,
          probi: '',
          type: 'ugp'
        }
      ]

      expect(assertion).toEqual({
        rewardsData: expectedState
      })
    })
  })

  describe('ON_GRANT_FINISH', () => {
    it('does not modify state if there are no grants available', () => {
      const assertion = reducers(undefined, {
        type: types.ON_GRANT_FINISH,
        payload: {
          properties: {
            status: 0,
            expiryTime: 11000,
            probi: '30.000000'
          }
        }
      })

      const expectedState: Rewards.State = { ...defaultState }

      expect(assertion).toEqual({
        rewardsData: expectedState
      })
    })
    it('modifies correct grant with updated status info (0)', () => {
      const initialState = {
        ...defaultState
      }
      initialState.grants = [
        {
          promotionId: 'test-promotion-id',
          expiryTime: 0,
          probi: '',
          type: 'ugp'
        },
        {
          promotionId: 'test-promotion-id-2',
          expiryTime: 0,
          probi: '',
          type: 'ads',
          captcha: 'data:image/jpeg;base64,XXX',
          hint: 'blue'
        }
      ]
      initialState.currentGrant = {
        promotionId: 'test-promotion-id-2',
        expiryTime: 0,
        probi: '',
        type: 'ads'
      }

      const assertion = reducers({
        rewardsData: initialState
      }, {
        type: types.ON_GRANT_FINISH,
        payload: {
          properties: {
            status: 0,
            expiryTime: 11,
            probi: '30.000000'
          }
        }
      })

      const expectedState: Rewards.State = { ...defaultState }
      expectedState.currentGrant = {
        promotionId: 'test-promotion-id-2',
        expiryTime: 0,
        probi: '',
        type: 'ads'
      }
      expectedState.grants = [
        {
          promotionId: 'test-promotion-id',
          expiryTime: 0,
          probi: '',
          type: 'ugp'
        },
        {
          promotionId: 'test-promotion-id-2',
          expiryTime: 11000,
          probi: '30.000000',
          type: 'ads',
          captcha: 'data:image/jpeg;base64,XXX',
          hint: 'blue',
          status: null
        }
      ]
      expectedState.ui.emptyWallet = false

      expect(assertion).toEqual({
        rewardsData: expectedState
      })
    })
    it('modifies correct grant with updated status info (6)', () => {
      const initialState = {
        ...defaultState
      }
      initialState.grants = [
        {
          promotionId: 'test-promotion-id',
          expiryTime: 0,
          probi: '',
          type: 'ugp'
        },
        {
          promotionId: 'test-promotion-id-2',
          expiryTime: 0,
          probi: '',
          type: 'ads',
          captcha: 'data:image/jpeg;base64,XXX',
          hint: 'blue'
        }
      ]
      initialState.currentGrant = {
        promotionId: 'test-promotion-id-2',
        expiryTime: 0,
        probi: '',
        type: 'ads'
      }

      const assertion = reducers({
        rewardsData: initialState
      }, {
        type: types.ON_GRANT_FINISH,
        payload: {
          properties: {
            status: 6
          }
        }
      })

      const expectedState: Rewards.State = { ...defaultState }
      expectedState.currentGrant = {
        promotionId: 'test-promotion-id-2',
        expiryTime: 0,
        probi: '',
        type: 'ads'
      }
      expectedState.grants = [
        {
          promotionId: 'test-promotion-id',
          expiryTime: 0,
          probi: '',
          type: 'ugp'
        },
        {
          promotionId: 'test-promotion-id-2',
          expiryTime: 0,
          probi: '',
          type: 'ads',
          captcha: 'data:image/jpeg;base64,XXX',
          hint: 'blue',
          status: 'wrongPosition'
        }
      ]
      expectedState.ui.emptyWallet = false

      expect(assertion).toEqual({
        rewardsData: expectedState
      })
    }) 
  })
})
