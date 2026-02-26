/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { RewardsActions, defaultRewardsStore } from '../state/rewards_store'

export function createRewardsStore() {
  const store = defaultRewardsStore()

  store.update({
    initialized: true,
    rewardsFeatureEnabled: true,
    rewardsEnabled: true,
    showRewardsWidget: true,
    rewardsBalance: null,
    rewardsExchangeRate: 1,
    rewardsExternalWallet: {
      provider: 'gemini',
      authenticated: true,
      name: 'Joe',
      url: 'https://brave.com',
    },
    minEarningsPreviousMonth: 0,
    payoutStatus: {},
    tosUpdateRequired: false,
    rewardsAdsViewed: 1,
  })

  setTimeout(() => {
    store.update({ rewardsBalance: 1.204 })
  }, 2000)

  const actions: RewardsActions = {
    ...store.getState().actions,

    setShowRewardsWidget(showRewardsWidget) {
      store.update({ showRewardsWidget })
    },
  }

  store.update({ actions })

  return store
}
