/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'

// Constant
// Temporary (ryanml)
import { types } from '../constants/rewards_types'

const getPromotion = (id?: string, promotions?: Rewards.Promotion[]) => {
  if (!id || !promotions) {
    return null
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
    case types.GET_GRANT_CAPTCHA:
      if (!state.promotions) {
        break
      }

      const currentPromotion = getPromotion(action.payload.promotionId, state.promotions)

      if (!currentPromotion) {
        break
      }

      state.currentPromotion = currentPromotion
      if (currentPromotion.promotionId && currentPromotion.type) {
        chrome.send('brave_rewards.getGrantCaptcha', [currentPromotion.promotionId, currentPromotion.type])
      }
      break
    case types.ON_GRANT_CAPTCHA: {
      if (state.currentPromotion && state.promotions) {
        const props = action.payload.captcha
        let hint = props.hint
        let captcha = `data:image/jpeg;base64,${props.image}`

        const promotions = state.promotions.map((item: Rewards.Promotion) => {
          let promotion = item
          let promotionId = state.currentPromotion && state.currentPromotion.promotionId

          if (promotionId === item.promotionId) {
            promotion = item
            promotion.captcha = captcha
            promotion.hint = hint
          }

          return promotion
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
        chrome.send('brave_rewards.solveGrantCaptcha', [JSON.stringify({
          x: action.payload.x,
          y: action.payload.y
        }), promotionId])
      }
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
    case types.ON_GRANT_FINISH: {
      state = { ...state }
      let newPromotion: any = {}
      const properties: Rewards.Promotion = action.payload.properties

      if (!state.promotions) {
        break
      }

      if (!state.currentPromotion) {
        state.firstLoad = false
        state.promotions = []
        let ui = state.ui
        chrome.send('brave_rewards.fetchPromotions', ['', ''])

        if (properties.status === 0) {
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

      switch (properties.status) {
        case 0:
          let ui = state.ui
          newPromotion.expiresAt = properties.expiresAt * 1000
          newPromotion.amount = properties.amount
          newPromotion.type = properties.type
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
          if (state.currentPromotion.promotionId && state.currentPromotion.type) {
            chrome.send('brave_rewards.getGrantCaptcha', [state.currentPromotion.promotionId, state.currentPromotion.type])
          }
          break
        case 13:
          newPromotion.status = 'grantGone'
          break
        case 18:
          newPromotion.status = 'grantAlreadyClaimed'
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
