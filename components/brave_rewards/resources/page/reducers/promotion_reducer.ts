/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'
import { types } from '../actions/rewards_types'
import { getCurrentBalanceReport } from './utils'
import * as Rewards from '../lib/types'

const getPromotion = (id: string, promotions?: Rewards.Promotion[]) => {
  if (!promotions) {
    return undefined
  }

  return promotions.find((promotion: Rewards.Promotion) => {
    return (promotion.promotionId === id)
  })
}

const updatePromotions = (newPromotion: Rewards.Promotion, promotions: Rewards.Promotion[]): Rewards.Promotion[] => {
  return promotions.map((promotion: Rewards.Promotion) => {
    if (newPromotion.promotionId === promotion.promotionId) {
      return Object.assign(promotion, newPromotion)
    }
    return promotion
  })
}

const updatePromotion = (newPromotion: Rewards.Promotion, promotions: Rewards.Promotion[]): Rewards.Promotion => {
  const oldPromotion = promotions.filter((promotion: Rewards.Promotion) => newPromotion.promotionId === promotion.promotionId)

  if (oldPromotion.length === 0) {
    return newPromotion
  }

  return Object.assign(oldPromotion[0], newPromotion)
}

const promotionReducer: Reducer<Rewards.State | undefined> = (state: Rewards.State, action) => {
  if (!state) {
    return
  }

  const payload = action.payload
  switch (action.type) {
    case types.FETCH_PROMOTIONS: {
      chrome.send('brave_rewards.fetchPromotions')
      break
    }
    case types.ON_PROMOTIONS: {
      state = { ...state }
      if (payload.properties.result === 1) {
        break
      }

      if (!state.promotions) {
        state.promotions = []
      }

      let promotions = payload.properties.promotions

      if (!promotions || promotions.length === 0) {
        state.promotions = []
        break
      }

      const toMilliseconds = 1000
      promotions = promotions.map((promotion: Rewards.Promotion) => {
        promotion.expiresAt = promotion.expiresAt * toMilliseconds
        promotion.claimableUntil = promotion.claimableUntil * toMilliseconds
        promotion.createdAt = promotion.createdAt * toMilliseconds
        return updatePromotion(promotion, state.promotions || [])
      })

      state = {
        ...state,
        promotions
      }

      break
    }
    case types.CLAIM_PROMOTION: {
      const promotionId = action.payload.promotionId
      if (!promotionId) {
        break
      }
      chrome.send('brave_rewards.claimPromotion', [promotionId])
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

      let promotions = state.promotions.map((item: Rewards.Promotion) => {
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
    case types.ON_PROMOTION_CLAIM_STARTED: {
      if (state.promotions) {
        const promotion = getPromotion(payload.promotionId, state.promotions)
        if (promotion) {
          state = {
            ...state,
            promotions: updatePromotions({
              ...promotion,
              captchaStatus: 'start'
            }, state.promotions)
          }
        }
      }
      break
    }
    case types.ON_PROMOTION_FINISH: {
      state = { ...state }
      let newPromotion: any = {}
      const result = payload.properties.result

      if (!state.promotions) {
        break
      }

      let promotionId = payload.properties.promotion.promotionId
      newPromotion.promotionId = promotionId

      const currentPromotion = getPromotion(promotionId, state.promotions)

      switch (result) {
        case 0:
          let ui = state.ui
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

          state = {
            ...state,
            ui
          }

          chrome.send('brave_rewards.fetchBalance')
          chrome.send('brave_rewards.fetchPromotions')
          getCurrentBalanceReport()
          break
        case 6:
          newPromotion.captchaStatus = 'wrongPosition'
          if (!promotionId) {
            break
          }
          chrome.send('brave_rewards.claimPromotion', [promotionId])
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

export default promotionReducer
