/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { ExternalWallet } from '../../../../components/brave_rewards/resources/shared/lib/external_wallet'

export interface RewardsState {
  initialized: boolean
  rewardsFeatureEnabled: boolean
  showRewardsWidget: boolean
  rewardsEnabled: boolean
  rewardsExternalWallet: ExternalWallet | null
  rewardsBalance: number | null
  rewardsExchangeRate: number
  rewardsAdsViewed: number | null
}

export function defaultRewardsState(): RewardsState {
  return {
    initialized: false,
    rewardsFeatureEnabled: false,
    showRewardsWidget: false,
    rewardsEnabled: false,
    rewardsExternalWallet: null,
    rewardsBalance: null,
    rewardsExchangeRate: 0,
    rewardsAdsViewed: null
  }
}

export interface RewardsActions {
  setShowRewardsWidget: (showRewardsWidget: boolean) => void
}

export function defaultRewardsActions(): RewardsActions {
  return {
    setShowRewardsWidget(showRewardsWidget) {}
  }
}
