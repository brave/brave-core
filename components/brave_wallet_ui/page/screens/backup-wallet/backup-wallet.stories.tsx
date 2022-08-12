// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// mocks
import { mockedMnemonic } from '../../../stories/mock-data/user-accounts'

// components
import { WalletPageStory } from '../../../stories/wrappers/wallet-page-story-wrapper'
import { BackupWallet } from './backup-wallet'

export const _BackupWallet = () => {
  const complete = () => {
    alert('Wallet Setup Complete!!!')
  }

  return (
    <WalletPageStory pageStateOverride={{ mnemonic: mockedMnemonic }}>
      <BackupWallet
        onCancel={complete}
        isOnboarding={true}
      />
    </WalletPageStory>
  )
}

_BackupWallet.story = {
  name: 'BackupWallet'
}

export default _BackupWallet
