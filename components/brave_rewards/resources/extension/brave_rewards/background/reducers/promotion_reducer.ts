/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { types } from '../../constants/rewards_panel_types'
import * as storage from '../storage'

const getPromotion = (id?: string, promotions?: RewardsExtension.Promotion[]) => {
  if (!id || !promotions) {
    return null
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

  switch (action.type) {
    case types.FETCH_PROMOTIONS: {
      chrome.braveRewards.fetchPromotions()
      break
    }
    case types.ON_PROMOTION: {
      state = { ...state }
      const promotion = action.payload.properties
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
    case types.GET_GRANT_CAPTCHA:
      if (!state.promotions) {
        break
      }

      const currentPromotion = getPromotion(action.payload.promotionId, state.promotions)

      if (!currentPromotion) {
        break
      }

      state.currentPromotion = currentPromotion
      if (currentPromotion.promotionId) {
        // TODO implement
        //chrome.braveRewards.getGrantCaptcha(currentPromotion.promotionId, currentPromotion.type)
      }
      break
    case types.ON_GRANT_CAPTCHA: {
      if (state.currentPromotion && state.promotions) {
        const props = action.payload.captcha
        let hint = props.hint
        let captcha = `data:image/jpeg;base64,${props.image}`

        const promotions = state.promotions.map((item: RewardsExtension.Promotion) => {
          let promotionId
          let newPromotion = item

          if (!state ||
              !state.currentPromotion ||
              !state.currentPromotion.promotionId) {
            return newPromotion
          }

          promotionId = state.currentPromotion.promotionId

          if (promotionId === item.promotionId) {
            newPromotion = item
            newPromotion.captcha = captcha
            newPromotion.hint = hint
          }

          return newPromotion
        })

        state = {
          ...state,
          promotions
        }
      }
      break
    }
    case types.SOLVE_GRANT_CAPTCHA: {
      const promotionId = state.currentPromotion && state.currentPromotion.promotionId

      if (promotionId && action.payload.x && action.payload.y) {
        chrome.braveRewards.solveGrantCaptcha(JSON.stringify({
          x: action.payload.x,
          y: action.payload.y
        }), promotionId)
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
    case types.ON_GRANT_FINISH: {
      state = { ...state }
      let currentPromotion: any = state.currentPromotion
      const properties: RewardsExtension.Promotion = action.payload.properties

      if (!state.promotions || !state.currentPromotion) {
        break
      }

      switch (properties.status) {
        case 0:
          currentPromotion.expiresAt = properties.expiresAt * 1000
          currentPromotion.amount = properties.amount
          currentPromotion.type = properties.type
          currentPromotion.status = null
          break
        case 6:
          currentPromotion.status = 'wrongPosition'
          if (currentPromotion.promotionId && currentPromotion.type) {
            chrome.braveRewards.getGrantCaptcha(currentPromotion.promotionId, currentPromotion.type)
          }
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
