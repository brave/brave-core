/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'

// Constant
// Temporary (ryanml)
import { types } from '../constants/rewards_types'

const getPromotion = (id: string, promotions?: Rewards.Promotion[]) => {
  if (!promotions) {
    return undefined
  }

  return promotions.find((promotion: Rewards.Promotion) => {
    return (promotion.promotionId === id)
  })
}

const updatePromotion = (newPromotion: Rewards.Promotion, promotions: Rewards.Promotion[]) => {
  return promotions.map((promotion: Rewards.Promotion) => {
    if (newPromotion.promotionId === promotion.promotionId) {
      return Object.assign(promotion, newPromotion)
    }
    return promotion
  })
}

const promotionReducer: Reducer<Rewards.State | undefined> = (state: Rewards.State, action) => {
  const payload = action.payload
  switch (action.type) {
    case types.FETCH_PROMOTIONS: {
      chrome.send('brave_rewards.fetchPromotions')
      break
    }
    case types.ON_PROMOTION: {
      state = { ...state }
      if (payload.properties.status === 1) {
        break
      }

      if (!state.promotions) {
        state.promotions = []
      }

      const promotionId = payload.properties.promotionId

      if (!getPromotion(promotionId, state.promotions)) {
        state.promotions.push({
          promotionId: promotionId,
          expiresAt: 0,
          amount: 0,
          type: payload.properties.type
        })
      }

      state = {
        ...state,
        promotions: state.promotions
      }

      break
    }
    case types.CLAIM_PROMOTION: {
      const promotionId = action.payload.promotionId
      if (!promotionId) {
        state = {
          ...state,
          currentPromotion: undefined
        }
        break
      }
      chrome.send('brave_rewards.claimPromotion', [promotionId])
      break
    }
    case types.ON_CLAIM_PROMOTION: {
      const promotionId = payload.properties.promotionId
      if (!state.promotions || !promotionId) {
        break
      }

      let currentPromotion = state.currentPromotion
      if (!state.currentPromotion) {
        currentPromotion = getPromotion(promotionId, state.promotions)
      }

      if (!currentPromotion || payload.properties.result !== 0) {
        state = {
          ...state,
          currentPromotion: undefined
        }
        break
      }

      const hint = payload.properties.hint
      const captchaImage = payload.properties.captchaImage
      const captchaId = payload.properties.captchaId

      const promotions = state.promotions.map((item: Rewards.Promotion) => {
        if (promotionId === item.promotionId) {
          item.captchaImage = captchaImage
          item.captchaId = captchaId
          item.hint = hint
        }
        return item
      })

      state = {
        ...state,
        promotions,
        currentPromotion
      }
      break
    }
    case types.ATTEST_PROMOTION: {
      const promotionId = state.currentPromotion && state.currentPromotion.promotionId

      if (!promotionId || !payload.x || !payload.y) {
        break
      }

      const currentPromotion = getPromotion(promotionId, state.promotions)

      if (!currentPromotion || !currentPromotion.captchaId) {
        break
      }

      chrome.send('brave_rewards.attestPromotion', [promotionId, JSON.stringify({
        captchaId: currentPromotion.captchaId,
        x: parseInt(payload.x, 10),
        y: parseInt(payload.y, 10)
      })])
      break
    }
    case types.ON_GRANT_RESET: {
      if (state.currentPromotion && state.promotions) {
        let currentPromotion: any = state.currentPromotion

        const promotions = state.promotions.map((item: Rewards.Promotion) => {
          if (currentPromotion.promotionId === item.promotionId) {
            return {
              promotionId: currentPromotion.promotionId,
              amount: currentPromotion.amount,
              expiresAt: currentPromotion.expiresAt,
              type: currentPromotion.type
            }
          }
          return item
        })

        currentPromotion = undefined

        state = {
          ...state,
          promotions,
          currentPromotion
        }
      }
      break
    }
    case types.ON_GRANT_DELETE: {
      if (state.currentPromotion && state.promotions) {
        let promotionIndex = -1
        let currentPromotion: any = state.currentPromotion

        state.promotions.map((item: Rewards.Promotion, i: number) => {
          if (currentPromotion.promotionId === item.promotionId) {
            promotionIndex = i
          }
        })

        if (promotionIndex > -1) {
          state.promotions.splice(promotionIndex, 1)
          currentPromotion = undefined
        }

        state = {
          ...state,
          currentPromotion
        }
      }
      break
    }
    case types.ON_PROMOTION_FINISH: {
      state = { ...state }
      let newPromotion: any = {}
      const promotion: Rewards.Promotion = payload.properties.promotion
      const result = payload.properties.result

      if (!state.promotions) {
        break
      }

      if (!state.currentPromotion) {
        state.firstLoad = false
        state.promotions = []
        let ui = state.ui
        chrome.send('brave_rewards.fetchPromotions', ['', ''])

        if (result === 0) {
          ui.emptyWallet = false
          chrome.send('brave_rewards.getWalletProperties')
          chrome.send('brave_rewards.fetchBalance')
        }

        state = {
          ...state,
          ui
        }
        break
      }

      newPromotion.promotionId = state.currentPromotion.promotionId

      switch (result) {
        case 0:
          let ui = state.ui
          newPromotion.expiresAt = promotion.expiresAt * 1000
          newPromotion.amount = promotion.amount
          newPromotion.type = promotion.type
          newPromotion.status = null
          ui.emptyWallet = false

          state = {
            ...state,
            ui
          }

          chrome.send('brave_rewards.getWalletProperties')
          chrome.send('brave_rewards.fetchBalance')
          break
        case 6:
          newPromotion.status = 'wrongPosition'
          if (!state.currentPromotion.promotionId) {
            break
          }
          const promotionId = state.currentPromotion.promotionId
          if (!promotionId) {
            state = {
              ...state,
              currentPromotion: undefined
            }
            break
          }
          chrome.send('brave_rewards.claimPromotion', [promotionId])
          break
        default:
          newPromotion.status = 'generalError'
          break
      }

      if (state.promotions) {
        const promotions = updatePromotion(newPromotion, state.promotions)

        state = {
          ...state,
          promotions
        }
      }

      break
    }
  }
  return state
}

export default promotionReducer
