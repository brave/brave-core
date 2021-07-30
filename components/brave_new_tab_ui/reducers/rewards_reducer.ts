/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'
import { types } from '../constants/rewards_types'
import { getTotalContributions } from '../rewards-utils'
import { InitialRewardsData, PreInitialRewardsData } from '../api/initialData'

const rewardsReducer: Reducer<NewTab.State | undefined> = (state: NewTab.State, action) => {
  const payload = action.payload

  switch (action.type) {
    case types.ON_ADS_ENABLED:
      state = { ...state }
      state.rewardsState.enabledAds = payload.enabled
      break

    case types.ON_ADS_ACCOUNT_STATEMENT:
      state = { ...state }
      state.rewardsState.adsAccountStatement = payload.adsAccountStatement
      break

    case types.ON_BALANCE_REPORT:
      state = { ...state }
      const report = payload.report || {}
      state.rewardsState.totalContribution = getTotalContributions(report)
      break

    case types.DISMISS_NOTIFICATION:
      state = { ...state }
      const dismissedNotifications = state.rewardsState.dismissedNotifications
      dismissedNotifications.push(payload.id)
      state.rewardsState.dismissedNotifications = dismissedNotifications

      state.rewardsState.promotions = state.rewardsState.promotions.filter((promotion) => {
        return promotion.promotionId !== payload.id
      })
      break

    case types.ON_PROMOTIONS: {
      if (action.payload.result === 1) {
        break
      }

      const promotions = payload.promotions

      state = { ...state }

      if (!state.rewardsState.promotions) {
        state.rewardsState.promotions = []
      }

      promotions.forEach((promotion: NewTab.Promotion) => {
        if (!state || !state.rewardsState) {
          return
        }

        if (!state.rewardsState.dismissedNotifications) {
          state.rewardsState.dismissedNotifications = []
        }

        if (state.rewardsState.dismissedNotifications.indexOf(promotion.promotionId) > -1) {
          return
        }

        const hasPromotion = state.rewardsState.promotions.find((promo: NewTab.Promotion) => {
          return promo.promotionId === promotion.promotionId
        })
        if (hasPromotion) {
          return
        }

        const updatedPromotions = state.rewardsState.promotions
        updatedPromotions.push({
          promotionId: promotion.promotionId,
          type: promotion.type
        })

        state.rewardsState.promotions = updatedPromotions
      })

      break
    }

    case types.ON_PROMOTION_FINISH:
      if (payload.result !== 0) {
        break
      }

      if (!state.rewardsState.promotions) {
        state.rewardsState.promotions = []
      }

      state = { ...state }
      const oldNotifications = state.rewardsState.dismissedNotifications

      oldNotifications.push(payload.promotion.promotionId)
      state.rewardsState.dismissedNotifications = oldNotifications

      state.rewardsState.promotions = state.rewardsState.promotions.filter((promotion: NewTab.Promotion) => {
        return promotion.promotionId !== payload.promotion.promotionId
      })
      break

    case types.ON_BALANCE:
      state = { ...state }
      state.rewardsState.balance = payload.balance
      break

    case types.SET_PRE_INITIAL_REWARDS_DATA:
      const preInitialRewardsDataPayload = payload as PreInitialRewardsData
      state = {
        ...state,
        rewardsState: {
          ...state.rewardsState,
          enabledAds: preInitialRewardsDataPayload.enabledAds,
          adsSupported: preInitialRewardsDataPayload.adsSupported
        }
      }
      break

    case types.SET_INITIAL_REWARDS_DATA:
      const initialRewardsDataPayload = payload as InitialRewardsData
      const newRewardsState = {
        balance: initialRewardsDataPayload.balance,
        totalContribution: getTotalContributions(initialRewardsDataPayload.report),
        parameters: initialRewardsDataPayload.parameters
      } as any

      if (payload.adsAccountStatement) {
        newRewardsState.adsAccountStatement = payload.adsAccountStatement
      }

      state = {
        ...state,
        rewardsState: {
          ...state.rewardsState,
          ...newRewardsState
        }
      }
      break
    case types.ON_COMPLETE_RESET:
      state = { ...state }
      state = {
        ...state,
        rewardsState: {
          ...state.rewardsState,
          enabledAds: false
        }
      }
      break

    default:
      break
  }

  return state
}

export default rewardsReducer
