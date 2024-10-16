// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'

// types
import {
  CreateAccountOptionsType,
  WalletRoutes
} from '../../../../constants/types'

// utils
import {
  CreateAccountOptions //
} from '../../../../options/create-account-options'
import { getLocale } from '../../../../../common/locale'

// components
import { AccountType } from './components/account_type'
import {
  OnboardingContentLayout //
} from '../components/onboarding_content_layout/content_layout'

// styles
import { Divider } from './components/account_type.style'

// queries
import { useGetVisibleNetworksQuery } from '../../../../common/slices/api.slice'

// selectors
import { WalletSelectors } from '../../../../common/selectors'
import { useSafeWalletSelector } from '../../../../common/hooks/use-safe-selector'

export const OnboardingConnectHardwareWallet = () => {
  // routing
  const history = useHistory()

  // redux
  const isBitcoinLedgerEnabled = useSafeWalletSelector(
    WalletSelectors.isBitcoinLedgerEnabled
  )

  // queries
  const { data: visibleNetworks = [] } = useGetVisibleNetworksQuery()

  const accountOptions = CreateAccountOptions({
    visibleNetworks,
    isBitcoinEnabled: isBitcoinLedgerEnabled,
    isZCashEnabled: false // No zcash hardware accounts by now.
  })

  // methods
  const onSelectAccountType = React.useCallback(
    (accountType: CreateAccountOptionsType) => () => {
      history.push(
        WalletRoutes.OnboardingHardwareWalletConnectSelectDevice.replace(
          ':accountTypeName?',
          accountType.name.toLowerCase()
        )
      )
    },
    [history]
  )

  return (
    <OnboardingContentLayout
      title={getLocale('braveWalletConnectHardwareWalletSelectBlockchain')}
      padding='56px 0 0'
      showBackButton={false}
    >
      {accountOptions.map((option, index) => {
        return (
          <React.Fragment key={option.name}>
            <AccountType
              title={option.name}
              description={option.description}
              icons={option.chainIcons || []}
              onClick={onSelectAccountType(option)}
            />
            {index !== accountOptions.length - 1 && <Divider />}
          </React.Fragment>
        )
      })}
    </OnboardingContentLayout>
  )
}
