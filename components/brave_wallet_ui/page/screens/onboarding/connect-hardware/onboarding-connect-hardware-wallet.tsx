// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { useHistory } from 'react-router'

// types
import { CreateAccountOptionsType, WalletRoutes } from '../../../../constants/types'

// components
import { WalletPageLayout } from "../../../../components/desktop"
import AddHardwareAccountModal from '../../../../components/desktop/popup-modals/add-account-modal/add-hardware-account-modal'

// styles
import { OnboardingWrapper } from "../onboarding.style"

export const OnboardingConnectHardwareWallet = () => {
  // routing
  const history = useHistory()

  // methods
  const onSelectAccountType = React.useCallback((accountType: CreateAccountOptionsType) => () => {
    history.push(WalletRoutes.OnboardingConnectHardwareWallet.replace(':accountTypeName?', accountType.name.toLowerCase()))
  }, [])

  return (
    <WalletPageLayout>
      <OnboardingWrapper>
        <AddHardwareAccountModal onSelectAccountType={onSelectAccountType} />
      </OnboardingWrapper>
    </WalletPageLayout>
  )
}
