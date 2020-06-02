/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { action } from 'typesafe-actions'

// Constants
import { types } from '../constants/rewards_internals_types'

export const getRewardsEnabled = () => action(types.GET_REWARDS_ENABLED)

export const onGetRewardsEnabled = (enabled: boolean) =>
  action(types.ON_GET_REWARDS_ENABLED, {
    enabled
  })

export const getRewardsInternalsInfo = () => action(types.GET_REWARDS_INTERNALS_INFO)

export const onGetRewardsInternalsInfo = (info: RewardsInternals.State) =>
  action(types.ON_GET_REWARDS_INTERNALS_INFO, {
    info
  })

export const getBalance = () => action(types.GET_BALANCE)

export const onBalance = (balance: RewardsInternals.Balance) =>
  action(types.ON_BALANCE, {
    balance
  })

export const getPromotions = () => action(types.GET_PROMOTIONS)

export const onPromotions = (promotions: RewardsInternals.Promotion[]) =>
  action(types.ON_PROMOTIONS, {
    promotions
  })

export const getLog = () => action(types.GET_LOG)

export const onGetLog = (log: string) =>
  action(types.ON_GET_LOG, {
    log
  })

export const clearLog = () => action(types.CLEAR_LOG)

export const onClearLog = () => action(types.ON_CLEAR_LOG)
