// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { BraveRewardsProxyOverrides } from '../../../constants/testing_types'
import type { BraveRewardsProxyInstance } from '../brave_rewards_api_proxy'

export class MockBraveRewardsProxy {
  overrides: BraveRewardsProxyOverrides = {
    rewardsEnabled: true,
    balance: 100.5,
    externalWallet: null
  }

  applyOverrides = (overrides?: BraveRewardsProxyOverrides) => {
    if (!overrides) {
      return
    }

    this.overrides = { ...this.overrides, ...overrides }
  }

  getRewardsEnabled = async () => {
    return this.overrides.rewardsEnabled
  }

  fetchBalance = async () => {
    return this.overrides.balance
  }

  getExternalWallet = async () => {
    return this.overrides.externalWallet
  }
}

export type MockBraveRewardsProxyInstance = InstanceType<
  typeof MockBraveRewardsProxy
>

let braveRewardsProxyInstance: MockBraveRewardsProxyInstance

export const getBraveRewardsProxy = () => {
  if (!braveRewardsProxyInstance) {
    braveRewardsProxyInstance = new MockBraveRewardsProxy()
  }

  return braveRewardsProxyInstance as unknown as BraveRewardsProxyInstance &
    MockBraveRewardsProxy
}

export function resetRewardsProxy(
  overrides?: BraveRewardsProxyOverrides | undefined
) {
  braveRewardsProxyInstance = new MockBraveRewardsProxy()
  braveRewardsProxyInstance.applyOverrides(overrides)
}
