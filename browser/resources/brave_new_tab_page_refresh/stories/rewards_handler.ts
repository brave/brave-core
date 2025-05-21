/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { Store } from '../lib/store'
import { optional } from '../lib/optional'

import {
  RewardsState,
  RewardsActions,
  defaultRewardsActions } from '../state/rewards_state'

export function createRewardsHandler(
  store: Store<RewardsState>
): RewardsActions {
  store.update({
    rewardsFeatureEnabled: true,
    rewardsEnabled: true,
    showRewardsWidget: true,
    rewardsBalance: optional(1.204),
    rewardsExchangeRate: 1,
    rewardsExternalWallet: {
      provider: 'uphold',
      authenticated: true,
      name: 'Joe',
      url: 'https://brave.com'
    }
  })

  return {
    ...defaultRewardsActions(),

    setShowRewardsWidget(showRewardsWidget) {
      store.update({ showRewardsWidget })
    }
  }
}
