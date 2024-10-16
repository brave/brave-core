// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Redirect, useHistory, useParams } from 'react-router'

// utils
import { BraveWallet, WalletRoutes } from '../../../../constants/types'
import {
  CreateAccountOptions //
} from '../../../../options/create-account-options'

// components
import {
  OnboardingContentLayout //
} from '../components/onboarding_content_layout/content_layout'
import {
  HardwareWalletConnect //
} from '../../../../components/desktop/hardware-wallet-connect/hardware_wallet_connect'
import { getLocale } from '../../../../../common/locale'

// queries
import { useGetVisibleNetworksQuery } from '../../../../common/slices/api.slice'

// selectors
import { useSafeWalletSelector } from '../../../../common/hooks/use-safe-selector'
import { WalletSelectors } from '../../../../common/selectors'

interface Params {
  accountTypeName: string
}

export const OnboardingSelectWalletDevice = () => {
  // routing
  const { accountTypeName } = useParams<Params>()
  const history = useHistory()

  // queries
  const { data: visibleNetworks = [] } = useGetVisibleNetworksQuery()

  // redux
  const isBitcoinLedgerEnabled = useSafeWalletSelector(
    WalletSelectors.isBitcoinLedgerEnabled
  )

  const accountOptions = CreateAccountOptions({
    visibleNetworks,
    isBitcoinEnabled: isBitcoinLedgerEnabled,
    isZCashEnabled: false // No zcash hardware accounts by now.
  })

  // state
  const [selectedHardwareWallet, setSelectedHardwareWallet] =
    React.useState<BraveWallet.HardwareVendor>()

  const selectedAccountType =
    accountTypeName !== ''
      ? accountOptions.find((option) => {
          return option.name.toLowerCase() === accountTypeName?.toLowerCase()
        })
      : undefined

  if (!selectedAccountType) {
    return <Redirect to={WalletRoutes.OnboardingHardwareWalletConnect} />
  }

  const pageTitle = selectedHardwareWallet
    ? getLocale('braveWalletAuthorizeHardwareWallet')
    : getLocale('braveWalletConnectHardwareTitle')

  return (
    <OnboardingContentLayout
      title={pageTitle}
      centerContent={true}
    >
      <HardwareWalletConnect
        selectedAccountType={selectedAccountType}
        onSelectVendor={setSelectedHardwareWallet}
        onSuccess={() => history.push(WalletRoutes.OnboardingComplete)}
      />
    </OnboardingContentLayout>
  )
}
