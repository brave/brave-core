// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// components
import { WalletPageStory } from '../../../../stories/wrappers/wallet-page-story-wrapper'
import { OnboardingWelcome } from './onboarding-welcome'

export const _OnboardingWelcome = () => {
  return <WalletPageStory>
    <OnboardingWelcome />
  </WalletPageStory>
}

_OnboardingWelcome.story = {
  name: 'Welcome'
}

export default _OnboardingWelcome
