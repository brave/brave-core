// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Meta } from '@storybook/react'

// Components
import {
  WalletPageStory, //
} from '$wallet/stories/wrappers/wallet-page-story-wrapper'
import { OnboardingNetworkSelection } from './onboarding_network_selection'

export const OnboardingNetworkSelectionStory = () => {
  return (
    <WalletPageStory>
      <OnboardingNetworkSelection />
    </WalletPageStory>
  )
}

export default {
  title: 'Wallet/Desktop/Screens/OnboardingV2',
  component: OnboardingNetworkSelectionStory,
} as Meta<typeof OnboardingNetworkSelectionStory>
