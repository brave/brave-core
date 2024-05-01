// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import {
  WalletPageStory //
} from '../../../../stories/wrappers/wallet-page-story-wrapper'
import { OnboardingCreatingWallet } from './onboarding_creating_wallet'

export const _OnboardingCreatingWallet = () => {
  return (
    <WalletPageStory>
      <OnboardingCreatingWallet />
    </WalletPageStory>
  )
}

_OnboardingCreatingWallet.story = {
  name: 'Creating Wallet'
}

export default _OnboardingCreatingWallet
