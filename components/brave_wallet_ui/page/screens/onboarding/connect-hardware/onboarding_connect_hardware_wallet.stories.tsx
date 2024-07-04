// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// components
import {
  WalletPageStory //
} from '../../../../stories/wrappers/wallet-page-story-wrapper'
import {
  OnboardingConnectHardwareWallet //
} from './onboarding_connect_hardware_wallet'

export const _OnboardingConnectHardwareWallet = {
  title: 'Connect Hardware Wallet',
  render: () => {
    return (
      <WalletPageStory>
        <OnboardingConnectHardwareWallet />
      </WalletPageStory>
    )
  }
}

export default { component: OnboardingConnectHardwareWallet }
