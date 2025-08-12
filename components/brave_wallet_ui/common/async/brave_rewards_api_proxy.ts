// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { loadTimeData } from '$web-common/loadTimeData'
import {
  externalWalletFromExtensionData, //
} from '../../../brave_rewards/resources/shared/lib/external_wallet'
import { RewardsExternalWallet, WalletStatus } from '../../constants/types'
import { BraveRewardsProxyOverrides } from '../../constants/testing_types'
import { RewardsPageProxy } from '../../../brave_rewards/resources/rewards_page/webui/rewards_page_proxy'

export class BraveRewardsProxy {
  private rewardsPageProxy: RewardsPageProxy | null = null

  constructor() {
    if (loadTimeData.getBoolean('rewardsFeatureEnabled')) {
      this.rewardsPageProxy = RewardsPageProxy.getInstance()
    }
  }

  getRewardsEnabled = async () => {
    if (!this.rewardsPageProxy) {
      return false
    }
    const { paymentId } =
      await this.rewardsPageProxy.handler.getRewardsPaymentId()
    return Boolean(paymentId)
  }

  getExternalWallet = async () => {
    if (!this.rewardsPageProxy) {
      return null
    }
    const result = await this.rewardsPageProxy.handler.getExternalWallet()
    const externalWallet = externalWalletFromExtensionData(
      result?.externalWallet,
    )
    if (!externalWallet) {
      return null
    }
    return <RewardsExternalWallet>{
      ...externalWallet,
      status: externalWallet.authenticated
        ? WalletStatus.kConnected
        : WalletStatus.kLoggedOut,
    }
  }

  fetchBalance = async () => {
    if (!this.rewardsPageProxy) {
      return 0
    }
    const result = await this.rewardsPageProxy.handler.getAvailableBalance()
    return result?.balance ?? undefined
  }
}

export type BraveRewardsProxyInstance = InstanceType<typeof BraveRewardsProxy>

let braveRewardsProxyInstance: BraveRewardsProxyInstance

export const getBraveRewardsProxy = () => {
  if (!braveRewardsProxyInstance) {
    braveRewardsProxyInstance = new BraveRewardsProxy()
  }

  return braveRewardsProxyInstance
}

/** For testing */
export function resetRewardsProxy(
  overrides?: BraveRewardsProxyOverrides | undefined,
) {
  // no-op in production
}
