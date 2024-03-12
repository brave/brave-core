// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory, useParams } from 'react-router'

// utils
import {
  CreateAccountOptionsType,
  WalletRoutes
} from '../../../../constants/types'
import { CreateAccountOptions } from '../../../../options/create-account-options'
import { HardwareVendor } from '../../../../common/api/hardware_keyrings'

// components
import { OnboardingContentLayout } from '../components/onboarding-content-layout/onboarding-content-layout'
import HardwareWalletConnect from '../../../../components/desktop/popup-modals/add-account-modal/hardware-wallet-connect'
import { getLocale } from '../../../../../common/locale'

interface Params {
  accountTypeName: string
}

export const OnboardingSelectWalletDevice = () => {
  // routing
  const { accountTypeName } = useParams<Params>()
  const history = useHistory()

  // state
  const [selectedHardwareWallet, setSelectedHardwareWallet] =
    React.useState<HardwareVendor>()

  // memos
  const accountOptions = React.useMemo(() => {
    return CreateAccountOptions({
      isBitcoinEnabled: false, // No bitcoin hardware accounts by now.
      isZCashEnabled: false // No zcash hardware accounts by now.
    })
  }, [])

  const selectedAccountType: CreateAccountOptionsType | undefined =
    React.useMemo(() => {
      return accountOptions.find((option) => {
        return option.name.toLowerCase() === accountTypeName?.toLowerCase()
      })
    }, [accountOptions, accountTypeName])

  const pageTitle = React.useMemo(() => {
    return selectedHardwareWallet
      ? getLocale('braveWalletAuthorizeHardwareWallet')
      : getLocale('braveWalletConnectHardwareTitle')
  }, [selectedHardwareWallet])

  return (
    <OnboardingContentLayout title={pageTitle}>
      {selectedAccountType && (
        <HardwareWalletConnect
          selectedAccountType={selectedAccountType}
          onSelectVendor={setSelectedHardwareWallet}
          onSuccess={() => history.push(WalletRoutes.OnboardingComplete)}
        />
      )}
    </OnboardingContentLayout>
  )
}
