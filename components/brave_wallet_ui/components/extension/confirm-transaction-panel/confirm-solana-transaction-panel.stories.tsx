// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// components
import { WalletPanelStory } from '../../../stories/wrappers/wallet-panel-story-wrapper'
import { ConfirmSolanaTransactionPanel } from './confirm-solana-transaction-panel'

export const _ConfirmSolanaTransactionPanel = () => {
  return <WalletPanelStory
    walletStateOverride={{
      hasInitialized: true,
      isWalletCreated: true
    }}
    panelStateOverride={{
      hasInitialized: true
    }}
  >
    <ConfirmSolanaTransactionPanel />
  </WalletPanelStory>
}

_ConfirmSolanaTransactionPanel.story = {
  name: 'Confirm Solana Transaction Panel (Sign & Send)'
}

export default _ConfirmSolanaTransactionPanel
