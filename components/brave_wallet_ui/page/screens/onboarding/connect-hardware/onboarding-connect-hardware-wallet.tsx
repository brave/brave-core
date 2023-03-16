// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// components
import { WalletPageLayout } from "../../../../components/desktop"
import AddHardwareAccountModal from '../../../../components/desktop/popup-modals/add-account-modal/add-hardware-account-modal'

// styles
import { OnboardingWrapper } from "../onboarding.style"

export const OnboardingConnectHardwareWallet = () => {
  return (
    <WalletPageLayout>
      <OnboardingWrapper>
        <AddHardwareAccountModal isOnboarding={true}/>
      </OnboardingWrapper>
    </WalletPageLayout>
  )
}