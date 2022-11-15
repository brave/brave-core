// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

import './locale'
import WalletPageStory from './wrappers/wallet-page-story-wrapper'
import Container from '../page/container'

export default {
  title: 'Wallet/Desktop',
  argTypes: {
    onboarding: { control: { type: 'boolean', onboard: false } },
    locked: { control: { type: 'boolean', lock: false } }
  }
}

export const _DesktopWalletConcept = () => {
  return (
    <WalletPageStory
      walletStateOverride={{
        hasIncorrectPassword: false,
        isWalletCreated: true,
        isSolanaEnabled: true,
        isFilecoinEnabled: true
      }}
      pageStateOverride={{
        hasInitialized: true
      }}
    >
      <Container />
    </WalletPageStory>
  )
}

_DesktopWalletConcept.story = {
  name: 'Concept'
}

export const _WalletOnboardingConcept = () => {
  return (
    <WalletPageStory
      walletStateOverride={{
        hasIncorrectPassword: false,
        isSolanaEnabled: true,
        isFilecoinEnabled: true,
        isWalletBackedUp: false,
        isWalletCreated: false
      }}
      pageStateOverride={{
        isCryptoWalletsInitialized: true,
        isMetaMaskInitialized: true,
        setupStillInProgress: true,
        isImportWalletsCheckComplete: true
      }}
    >
      <Container />
    </WalletPageStory>
  )
}

_WalletOnboardingConcept.story = {
  name: 'Onboarding Concept'
}
