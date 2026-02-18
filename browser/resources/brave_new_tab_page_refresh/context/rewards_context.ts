/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { defaultRewardsStore } from '../state/rewards_store'
import { createUseStateHook } from '$web-common/state_store_hooks'

export const RewardsContext = React.createContext(defaultRewardsStore())

export const useRewardsState = createUseStateHook(RewardsContext)

export function useRewardsActions() {
  return useRewardsState((s) => s.actions)
}
