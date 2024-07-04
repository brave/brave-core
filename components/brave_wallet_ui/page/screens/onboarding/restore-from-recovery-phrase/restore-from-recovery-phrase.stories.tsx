// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { Meta } from '@storybook/react'
import { WalletPageStory } from '../../../../stories/wrappers/wallet-page-story-wrapper'
import { OnboardingRestoreFromExtension } from './restore-from-extension'
import { OnboardingRestoreFromRecoveryPhrase } from './restore-from-recovery-phrase'

export const _OnboardingRestoreFromRecoveryPhrase = {
  render: () => {
    return (
      <WalletPageStory>
        <OnboardingRestoreFromRecoveryPhrase />
      </WalletPageStory>
    )
  },
  title: 'Restore From Recovery Phrase'

}

export default {
  component: OnboardingRestoreFromExtension,
  render: (args) => {
    return <WalletPageStory>
      <OnboardingRestoreFromExtension restoreFrom={args.restoreFrom} />
    </WalletPageStory>
  }
} as Meta<typeof OnboardingRestoreFromExtension>


export const RestoreFromRecoveryPhrase = {
  render: () => {
    return <WalletPageStory>
      <OnboardingRestoreFromRecoveryPhrase />
    </WalletPageStory>
  }
}

export const RestoreFromMetamaskWallet = {
  args: {
    restoreFrom: 'metamask'
  }
}

export const RestoreFromLegacyWallet = {
  args: {
    restoreFrom: 'legacy'
  }
}

