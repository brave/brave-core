// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { WalletPageStory } from '../../../../stories/wrappers/wallet-page-story-wrapper'
import { OnboardingRestoreFromRecoveryPhrase } from './restore-from-recovery-phrase'

export const _OnboardingRestoreFromRecoveryPhrase = () => {
  return <WalletPageStory>
    <OnboardingRestoreFromRecoveryPhrase restoreFrom='seed' />
  </WalletPageStory>
}

_OnboardingRestoreFromRecoveryPhrase.story = {
  name: 'Restore From Recovery Phrase'
}

export const _OnboardingRestoreFromMetaMaskExtension = () => {
  return <WalletPageStory
    pageStateOverride={{
      isMetaMaskInitialized: true,
      isImportWalletsCheckComplete: true
    }}
  >
    <OnboardingRestoreFromRecoveryPhrase restoreFrom='metamask' />
  </WalletPageStory>
}

_OnboardingRestoreFromMetaMaskExtension.story = {
  name: 'Restore From MetaMask (Extension)'
}

export const _OnboardingRestoreFromMetaMaskSeed = () => {
  return <WalletPageStory
    pageStateOverride={{
      isMetaMaskInitialized: false,
      isImportWalletsCheckComplete: true
    }}
  >
    <OnboardingRestoreFromRecoveryPhrase restoreFrom='metamask-seed' />
  </WalletPageStory>
}

_OnboardingRestoreFromMetaMaskSeed.story = {
  name: 'Restore From MetaMask (Seed)'
}

export const _OnboardingRestoreFromLegacyWallet = () => {
  return <WalletPageStory
    pageStateOverride={{
      isMetaMaskInitialized: false,
      isImportWalletsCheckComplete: true,
      isCryptoWalletsInitialized: true
    }}
  >
    <OnboardingRestoreFromRecoveryPhrase restoreFrom='legacy' />
  </WalletPageStory>
}

_OnboardingRestoreFromLegacyWallet.story = {
  name: 'Restore From Legacy Wallet'
}

export default _OnboardingRestoreFromRecoveryPhrase
