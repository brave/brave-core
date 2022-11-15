// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// mocks
import {
  mockSolDappSignAllTransactionsRequest,
  mockSolDappSignAndSendTransactionRequest,
  mockSolDappSignTransactionRequest
} from '../../../common/constants/mocks'

// components
import { WalletPanelStory } from '../../../stories/wrappers/wallet-panel-story-wrapper'
import { SignTransactionPanel } from './sign-transaction-panel'

export const _SignAllSolanaTxPanel = () => {
  return <WalletPanelStory
    walletStateOverride={{
      selectedPendingTransaction: mockSolDappSignAndSendTransactionRequest
    }}
    panelStateOverride={{
      selectedPanel: 'signTransaction',
      signTransactionRequests: [mockSolDappSignTransactionRequest],
      signAllTransactionsRequests: [mockSolDappSignAllTransactionsRequest]
    }}
  >
    <SignTransactionPanel
      signMode='signAllTxs'
    />
  </WalletPanelStory>
}

_SignAllSolanaTxPanel.story = {
  name: 'Sign Solana All Transactions Panel'
}

export const _SignSolanaTxPanel = () => {
  return <WalletPanelStory
    walletStateOverride={{
      selectedPendingTransaction: mockSolDappSignAndSendTransactionRequest
    }}
    panelStateOverride={{
      selectedPanel: 'signTransaction',
      signTransactionRequests: [mockSolDappSignTransactionRequest],
      signAllTransactionsRequests: [mockSolDappSignAllTransactionsRequest]
    }}
  >
    <SignTransactionPanel
      signMode='signTx'
    />
  </WalletPanelStory>
}

_SignSolanaTxPanel.story = {
  name: 'Sign Solana Transaction Panel'
}

export default {
  parameters: {
    layout: 'centered'
  }
}
