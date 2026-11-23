// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Meta } from '@storybook/react'

// Components
import { WalletPageStory } from '$wallet/stories/wrappers/wallet-page-story-wrapper'
import { OnboardingWelcome } from './onboarding_welcome'

export const OnboardingWelcomeStory = () => {
  return (
    <WalletPageStory>
      <OnboardingWelcome />
    </WalletPageStory>
  )
}

export default {
  title: 'Wallet/Desktop/Screens/OnboardingV2',
  component: OnboardingWelcomeStory,
} as Meta<typeof OnboardingWelcomeStory>
