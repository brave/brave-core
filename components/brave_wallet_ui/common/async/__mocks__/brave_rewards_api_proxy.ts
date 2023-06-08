// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import type {
  BraveRewardsProxyInstance,
  RewardsExternalWallet
} from '../brave_rewards_api_proxy'

export type BraveRewardsProxyOverrides = Partial<{
  rewardsEnabled: boolean
  balance: number
  externalWallet: RewardsExternalWallet | null
}>

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

export const getMockedBraveRewardsProxy = () => {
  if (!braveRewardsProxyInstance) {
    braveRewardsProxyInstance = new MockBraveRewardsProxy()
  }

  return braveRewardsProxyInstance as unknown as BraveRewardsProxyInstance &
    MockBraveRewardsProxy
}
