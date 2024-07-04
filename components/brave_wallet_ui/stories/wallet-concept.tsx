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

export const _DesktopWalletConcept = {
  render: () => {
    return (
      <WalletPageStory
        walletStateOverride={{
          isWalletCreated: true
        }}
        pageStateOverride={{
          hasInitialized: true
        }}
        uiStateOverride={{
          isPanel: true
        }}
      >
        <Container />
      </WalletPageStory>
    )
  }
}

export const _WalletOnboardingConcept = {
  render: () => {
    return (
      <WalletPageStory
        walletStateOverride={{
          isWalletCreated: false
        }}
        pageStateOverride={{
          setupStillInProgress: true
        }}
      >
        <Container />
      </WalletPageStory>
    )
  }
}
