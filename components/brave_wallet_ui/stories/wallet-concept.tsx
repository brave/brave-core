// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

import './locale'
import WalletPageStory from './wrappers/wallet-page-story-wrapper'
import Container from '../page/container'
import {
  mockAccount,
  mockNativeBalanceRegistry,
  mockTokenBalanceRegistry,
} from '../common/constants/mocks'

type StorybookWalletConceptArgs = {
  isPanel: boolean
  isAndroid: boolean
}

const panelViewport = {
  walletPanel: {
    name: 'Wallet Panel',
    styles: {
      width: '390px',
      height: '650px',
    },
  },
  android: {
    name: 'Android',
    styles: {
      width: '360px',
      height: '740px',
    },
  },
}

export default {
  title: 'Wallet/Desktop',
  argTypes: {
    isPanel: { control: { type: 'boolean', onboard: false } },
    onboarding: { control: { type: 'boolean', onboard: false } },
    locked: { control: { type: 'boolean', lock: false } },
    isAndroid: { control: { type: 'boolean', android: false } },
  },
  parameters: {
    viewport: {
      viewports: {
        ...panelViewport,
      },
    },
  },
}

export const _DesktopWalletConcept = {
  render: (args: StorybookWalletConceptArgs) => {
    // Props
    const { isPanel, isAndroid } = args

    return (
      <WalletPageStory
        walletStateOverride={{
          isWalletCreated: true,
        }}
        pageStateOverride={{
          hasInitialized: true,
        }}
        uiStateOverride={{
          isPanel: isPanel,
          isAndroid: isAndroid,
        }}
        apiOverrides={{
          selectedAccountId: mockAccount.accountId,
          nativeBalanceRegistry: mockNativeBalanceRegistry,
          tokenBalanceRegistry: mockTokenBalanceRegistry,
        }}
      >
        <Container />
      </WalletPageStory>
    )
  },
}

export const _WalletOnboardingConcept = {
  render: () => {
    return (
      <WalletPageStory
        walletStateOverride={{
          isWalletCreated: false,
        }}
        pageStateOverride={{
          setupStillInProgress: true,
        }}
      >
        <Container />
      </WalletPageStory>
    )
  },
}
