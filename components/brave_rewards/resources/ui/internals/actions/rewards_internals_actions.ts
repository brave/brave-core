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

export const onReset = () => action(types.ON_RESET)
