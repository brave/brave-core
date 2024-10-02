/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { useAppState } from './app_model_context'
import { isSelfCustodyProvider, isExternalWalletProviderAllowed } from '../../shared/lib/external_wallet'

export function useShouldShowSelfCustodyInvite() {
  return useAppState((state) => {
    if (state.selfCustodyInviteDismissed) {
      return false
    }

    if (state.externalWallet) {
      return false
    }

    if (!state.rewardsParameters) {
      return false
    }

    const { countryCode } = state
    const { walletProviderRegions } = state.rewardsParameters

    for (const provider of state.externalWalletProviders) {
      if (isSelfCustodyProvider(provider)) {
        const regionInfo = walletProviderRegions[provider] || null
        if (isExternalWalletProviderAllowed(countryCode, regionInfo)) {
          return true
        }
      }
    }

    return false
  })
}
