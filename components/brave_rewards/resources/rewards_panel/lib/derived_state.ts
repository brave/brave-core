/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { HostState } from './interfaces'
import { isExternalWalletProviderAllowed, isSelfCustodyProvider } from '../../shared/lib/external_wallet'

export function canConnectAccount (state: HostState) {
  const { declaredCountry } = state
  const { externalWalletRegions } = state.options
  return state.externalWalletProviders.some((provider) => {
    const regionInfo = externalWalletRegions.get(provider) || null
    return isExternalWalletProviderAllowed(declaredCountry, regionInfo)
  })
}

export function shouldShowSelfCustodyInvite (state: HostState) {
  if (state.selfCustodyInviteDismissed) {
    return false
  }
  if (state.userType !== 'unconnected') {
    return false
  }
  const { declaredCountry } = state
  const { externalWalletRegions } = state.options
  return state.externalWalletProviders.some((provider) => {
    if (!isSelfCustodyProvider(provider)) {
      return false
    }
    const regionInfo = externalWalletRegions.get(provider) || null
    return isExternalWalletProviderAllowed(declaredCountry, regionInfo)
  })
}
