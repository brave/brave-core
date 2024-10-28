/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { createStore } from '../lib/store'

import {
  RewardsModel,
  RewardsModelState,
  defaultModel,
  defaultState } from '../models/rewards_model'

export function createRewardsModel(): RewardsModel {
  const store = createStore<RewardsModelState>({
    ...defaultState(),
    rewardsFeatureEnabled: true,
    rewardsEnabled: true,
    showRewardsWidget: true,
    externalWallet: {
      provider: 'uphold',
      authenticated: true,
      name: 'Joe',
      url: 'https://brave.com'
    }
  })

  return {
    ...defaultModel(),

    getState: store.getState,
    addListener: store.addListener,

    setShowRewardsWidget(showRewardsWidget) {
      store.update({ showRewardsWidget })
    }
  }
}
