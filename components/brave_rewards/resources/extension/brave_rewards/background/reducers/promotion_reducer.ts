/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { types } from '../../constants/rewards_panel_types'
import * as storage from '../storage'

const getPromotion = (id: string, promotions?: RewardsExtension.Promotion[]) => {
  if (!promotions) {
    return undefined
  }

  return promotions.find((promotion: RewardsExtension.Promotion) => {
    return (promotion.promotionId === id)
  })
}

const updatePromotion = (newPromotion: RewardsExtension.Promotion, promotions: RewardsExtension.Promotion[]) => {
  return promotions.map((promotion: RewardsExtension.Promotion) => {
    if (newPromotion.promotionId === promotion.promotionId) {
      return Object.assign(newPromotion, promotion)
    }
    return promotion
  })
}

export const promotionPanelReducer = (state: RewardsExtension.State | undefined, action: any) => {
  if (state === undefined) {
    state = storage.load()
  }
  const payload = action.payload
  switch (action.type) {
    case types.FETCH_PROMOTIONS: {
      chrome.braveRewards.fetchPromotions()
      break
    }
    case types.ON_PROMOTION: {
      state = { ...state }
      const promotion = payload.properties
      if (promotion.status === 1) {
        break
      }

      if (!state.promotions) {
        state.promotions = []
      }

      const promotionId = promotion.promotionId

      if (!getPromotion(promotionId, state.promotions)) {
        state.promotions.push({
          promotionId: promotionId,
          expiresAt: 0,
          amount: 0,
          type: promotion.type
        })
      }

      state = {
        ...state,
        promotions: state.promotions
      }

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

      const promotions = state.promotions.map((item: RewardsExtension.Promotion) => {
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
    case types.ON_GRANT_RESET: {
      if (state.currentPromotion && state.promotions) {
        let currentPromotion: any = state.currentPromotion

        const promotions = state.promotions.map((item: RewardsExtension.Promotion) => {
          if (currentPromotion.promotionId === item.promotionId) {
            return {
              promotionId: currentPromotion.promotionId,
              expiresAt: currentPromotion.expiresAt,
              amount: currentPromotion.amount,
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

        state.promotions.map((item: RewardsExtension.Promotion, i: number) => {
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
      let currentPromotion: any = state.currentPromotion
      const result = payload.result

      if (!state.promotions || !state.currentPromotion) {
        break
      }

      switch (result) {
        case 0: {
          const promotion: RewardsExtension.Promotion = payload.promotion
          currentPromotion.expiresAt = promotion.expiresAt * 1000
          currentPromotion.amount = promotion.amount
          currentPromotion.type = promotion.type
          currentPromotion.status = null
          break
        }
        case 6:
          currentPromotion.status = 'wrongPosition'
          break
        case 13:
          currentPromotion.status = 'grantGone'
          break
        case 18:
          currentPromotion.status = 'grantAlreadyClaimed'
          break
        default:
          currentPromotion.status = 'generalError'
          break
      }

      if (state.promotions) {
        const promotions = updatePromotion(currentPromotion, state.promotions)

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
