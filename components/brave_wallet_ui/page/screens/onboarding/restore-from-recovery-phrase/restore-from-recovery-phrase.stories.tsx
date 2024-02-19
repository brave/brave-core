// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { WalletPageStory } from '../../../../stories/wrappers/wallet-page-story-wrapper'
import { OnboardingRestoreFromExtension } from './restore-from-extension'
import { OnboardingRestoreFromRecoveryPhrase } from './restore-from-recovery-phrase-v3'

export const _OnboardingRestoreFromRecoveryPhrase = () => {
  return (
    <WalletPageStory>
      <OnboardingRestoreFromRecoveryPhrase />
    </WalletPageStory>
  )
}

_OnboardingRestoreFromRecoveryPhrase.story = {
  name: 'Restore From Recovery Phrase'
}

export const _OnboardingRestoreFromMetaMaskExtension = () => {
  return (
    <WalletPageStory>
      <OnboardingRestoreFromExtension restoreFrom='metamask' />
    </WalletPageStory>
  )
}

_OnboardingRestoreFromMetaMaskExtension.story = {
  name: 'Restore From MetaMask (Extension)'
}

export const _OnboardingRestoreFromLegacyWallet = () => {
  return (
    <WalletPageStory>
      <OnboardingRestoreFromExtension restoreFrom='legacy' />
    </WalletPageStory>
  )
}

_OnboardingRestoreFromLegacyWallet.story = {
  name: 'Restore From Legacy Wallet'
}

export default _OnboardingRestoreFromRecoveryPhrase
