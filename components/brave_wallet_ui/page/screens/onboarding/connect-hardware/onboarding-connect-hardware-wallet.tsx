// Copyright (c) 2023 The Brave Authors. All rights reserved.
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
import { CreateAccountOptions } from '../../../../options/create-account-options'

// components
import { AccountType } from './components/account-type'

// styles
import { OnboardingContentLayout } from '../components/onboarding-content-layout/onboarding-content-layout'
import { VerticalSpace } from '../../../../components/shared/style'
import { Divider } from './components/account-type.style'

export const OnboardingConnectHardwareWallet = () => {
  // routing
  const history = useHistory()

  // memos
  const createAccountOptions = React.useMemo(() => {
    return CreateAccountOptions({
      isBitcoinEnabled: false, // No bitcoin hardware accounts by now.
      isZCashEnabled: false // No zcash hardware accounts by now.
    })
  }, [])

  // methods
  const onSelectAccountType = React.useCallback(
    (accountType: CreateAccountOptionsType) => () => {
      history.push(
        WalletRoutes.OnboardingHardwareWalletConnect.replace(
          ':accountTypeName?',
          accountType.name.toLowerCase()
        )
      )
    },
    [history]
  )

  console.log(onSelectAccountType)

  return (
    <OnboardingContentLayout title='Select a blockchain to import your hardware wallet'>
      <VerticalSpace space='56px' />
      {createAccountOptions.map((option, index) => {
        return (
          <React.Fragment key={index}>
            <AccountType
              title={option.name}
              description={option.description}
              icons={option.chainIcons ? option.chainIcons : []}
              onClick={onSelectAccountType(option)}
            />
            {index !== createAccountOptions.length - 1 && <Divider />}
          </React.Fragment>
        )
      })}
    </OnboardingContentLayout>
  )
}
