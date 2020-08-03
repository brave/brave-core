/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { types } from '../../constants/rewards_panel_types'
import { Reducer } from 'redux'

const getPromotion = (id: string, promotions?: RewardsExtension.Promotion[]) => {
  if (!promotions) {
    return undefined
  }

  return promotions.find((promotion: RewardsExtension.Promotion) => {
    return (promotion.promotionId === id)
  })
}

const updatePromotions = (newPromotion: RewardsExtension.Promotion, promotions: RewardsExtension.Promotion[]) => {
  return promotions.map((promotion: RewardsExtension.Promotion) => {
    if (newPromotion.promotionId === promotion.promotionId) {
      return Object.assign(promotion, newPromotion)
    }
    return promotion
  })
}

const updatePromotion = (newPromotion: RewardsExtension.Promotion, promotions: RewardsExtension.Promotion[]): RewardsExtension.Promotion => {
  const oldPromotion = promotions.filter((promotion: RewardsExtension.Promotion) => newPromotion.promotionId === promotion.promotionId)

  if (oldPromotion.length === 0) {
    return newPromotion
  }

  return Object.assign(oldPromotion[0], newPromotion)
}

export const promotionPanelReducer: Reducer<RewardsExtension.State | undefined> = (state: RewardsExtension.State, action: any) => {
  if (!state) {
    return
  }

  const payload = action.payload
  switch (action.type) {
    case types.FETCH_PROMOTIONS: {
      chrome.braveRewards.fetchPromotions()
      break
    }
    case types.ON_PROMOTIONS: {
      state = { ...state }
      if (payload.result === 1) {
        break
      }

      if (!state.promotions) {
        state.promotions = []
      }

      let promotions = payload.promotions

      if (!promotions || promotions.length === 0) {
        state.promotions = []
        break
      }

      const toMilliseconds = 1000
      promotions = promotions.map((promotion: RewardsExtension.Promotion) => {
        if (!promotion || !state) {
          return promotion
        }

        promotion.expiresAt = promotion.expiresAt * toMilliseconds
        return updatePromotion(promotion, state.promotions || [])
      })

      state = {
        ...state,
        promotions
      }

      break
    }
    case types.ON_CLAIM_PROMOTION: {
      const promotionId = payload.properties.promotionId
      if (!state.promotions || !promotionId) {
        break
      }

      const hint = payload.properties.hint
      const captchaImage = payload.properties.captchaImage
      const captchaId = payload.properties.captchaId

      const promotions = state.promotions.map((item: RewardsExtension.Promotion) => {
        if (promotionId === item.promotionId) {
          if (item.captchaStatus !== 'wrongPosition') {
            item.captchaStatus = 'start'
          }
          item.captchaImage = captchaImage
          item.captchaId = captchaId
          item.hint = hint
        }
        return item
      })

      state = {
        ...state,
        promotions
      }
      break
    }
    case types.RESET_PROMOTION: {
      const promotionId = payload.promotionId
      if (!state.promotions || !promotionId) {
        break
      }

      const currentPromotion = getPromotion(promotionId, state.promotions)

      if (!currentPromotion) {
        break
      }

      const promotions = state.promotions.map((item: RewardsExtension.Promotion) => {
        if (currentPromotion.promotionId === item.promotionId) {
          item.captchaStatus = null
          item.captchaImage = ''
          item.captchaId = ''
          item.hint = ''
        }
        return item
      })

      state = {
        ...state,
        promotions
      }
      break
    }
    case types.DELETE_PROMOTION: {
      const promotionId = payload.promotionId
      if (!state.promotions || !promotionId) {
        break
      }
      const currentPromotion = getPromotion(promotionId, state.promotions)

      if (!currentPromotion) {
        break
      }

      let promotions = state.promotions.map((item: RewardsExtension.Promotion) => {
        if (currentPromotion.promotionId === item.promotionId) {
          item.captchaStatus = null
          item.status = 4
        }

        return item
      })

      state = {
        ...state,
        promotions
      }
      break
    }
    case types.ON_PROMOTION_FINISH: {
      state = { ...state }
      let newPromotion: any = {}
      const result = payload.result

      if (!state.promotions) {
        break
      }

      let promotionId = payload.promotion.promotionId
      newPromotion.promotionId = promotionId

      const currentPromotion = getPromotion(promotionId, state.promotions)

      switch (result) {
        case 0:
          if (currentPromotion) {
            if (currentPromotion.hint) {
              newPromotion.captchaStatus = 'finished'
            } else {
              newPromotion.status = 4
            }
          }
          newPromotion.captchaImage = ''
          newPromotion.captchaId = ''
          newPromotion.hint = ''
          break
        case 6:
          newPromotion.captchaStatus = 'wrongPosition'
          break
        default:
          newPromotion.captchaStatus = 'generalError'
          break
      }

      if (state.promotions) {
        const promotions = updatePromotions(newPromotion, state.promotions)

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
