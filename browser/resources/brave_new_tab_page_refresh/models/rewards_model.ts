/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { ExternalWallet } from '../../../../components/brave_rewards/resources/shared/lib/external_wallet'

export interface RewardsModelState {
  rewardsFeatureEnabled: boolean
  showRewardsWidget: boolean
  rewardsEnabled: boolean
  externalWallet: ExternalWallet | null
}

export function defaultState(): RewardsModelState {
  return {
    rewardsFeatureEnabled: false,
    showRewardsWidget: false,
    rewardsEnabled: false,
    externalWallet: null
  }
}

export interface RewardsModel {
  getState: () => RewardsModelState
  addListener: (listener: (state: RewardsModelState) => void) => () => void
  setShowRewardsWidget: (showRewardsWidget: boolean) => void
}

export function defaultModel(): RewardsModel {
  const state = defaultState()
  return {
    getState() { return state },
    addListener() { return () => {} },
    setShowRewardsWidget(showRewardsWidget) {}
  }
}
