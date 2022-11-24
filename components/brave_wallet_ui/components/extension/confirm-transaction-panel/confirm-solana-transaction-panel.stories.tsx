// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// mocks
import {
  mockSolanaTestnetNetwork,
  mockSolDappSignAndSendTransactionRequest
} from '../../../common/constants/mocks'

// components
import { WalletPanelStory } from '../../../stories/wrappers/wallet-panel-story-wrapper'
import { ConfirmSolanaTransactionPanel } from './confirm-solana-transaction-panel'

export const _ConfirmSolanaTransactionPanel = () => {
  return <WalletPanelStory
    walletStateOverride={{
      selectedNetwork: mockSolanaTestnetNetwork,
      hasInitialized: true,
      isWalletCreated: true,
      selectedPendingTransaction: mockSolDappSignAndSendTransactionRequest
    }}
    panelStateOverride={{
      hasInitialized: true
    }}
  >
    <ConfirmSolanaTransactionPanel
      onConfirm={() => {}}
      onReject={() => {}}
    />
  </WalletPanelStory>
}

_ConfirmSolanaTransactionPanel.story = {
  name: 'Confirm Solana Transaction Panel (Sign & Send)'
}

export default _ConfirmSolanaTransactionPanel
