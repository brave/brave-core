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

// components
import { AccountType } from './components/account-type'
import {
  OnboardingContentLayout //
} from '../components/onboarding-content-layout'

// styles
import { VerticalSpace } from '../../../../components/shared/style'
import { Divider } from './components/account-type.style'
import { getLocale } from '../../../../../common/locale'

const accountOptions = CreateAccountOptions({
  isBitcoinEnabled: false, // No bitcoin hardware accounts by now.
  isZCashEnabled: false // No zcash hardware accounts by now.
})

export const OnboardingConnectHardwareWallet = () => {
  // routing
  const history = useHistory()

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
    >
      <VerticalSpace space='56px' />
      {accountOptions.map((option, index) => {
        return (
          <React.Fragment key={index}>
            <AccountType
              title={option.name}
              description={option.description}
              icons={option.chainIcons ? option.chainIcons : []}
              onClick={onSelectAccountType(option)}
            />
            {index !== accountOptions.length - 1 && <Divider />}
          </React.Fragment>
        )
      })}
    </OnboardingContentLayout>
  )
}
