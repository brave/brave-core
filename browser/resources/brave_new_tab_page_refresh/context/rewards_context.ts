/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { defaultRewardsState } from '../state/rewards_state'
import { createRewardsHandler } from '../state/rewards_handler'
import { createStateProvider } from '../lib/state_provider'

export const RewardsProvider = createStateProvider(
  defaultRewardsState(),
  createRewardsHandler,
)

export const useRewardsState = RewardsProvider.useState
export const useRewardsActions = RewardsProvider.useActions
