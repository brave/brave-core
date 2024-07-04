// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// types
import { BraveWallet } from '../../../constants/types'

// mocks
import {
  mockSolDappSignAllTransactionsRequest,
  mockSolDappSignAndSendTransactionRequest,
  mockSolDappSignTransactionRequest
} from '../../../common/constants/mocks'

// utils
import {
  deserializeTransaction //
} from '../../../utils/model-serialization-utils'

// components
import {
  WalletPanelStory, //
  WalletPanelStoryProps
} from '../../../stories/wrappers/wallet-panel-story-wrapper'

import PendingSignatureRequestsPanel from './pending_signature_requests_panel'

const storyContextProps: WalletPanelStoryProps = {
  walletApiDataOverrides: {
    signTransactionRequests: [
      mockSolDappSignTransactionRequest,
      mockSolDappSignTransactionRequest
    ],
    signAllTransactionsRequests: [
      mockSolDappSignAllTransactionsRequest,
      mockSolDappSignAllTransactionsRequest
    ],
    transactionInfos: [
      deserializeTransaction(mockSolDappSignAndSendTransactionRequest),
      deserializeTransaction({
        ...mockSolDappSignAndSendTransactionRequest,
        txStatus: BraveWallet.TransactionStatus.Unapproved
      })
    ]
  },
  uiStateOverride: {
    selectedPendingTransactionId: mockSolDappSignAndSendTransactionRequest.id
  }
}

export const _PendingSolanaSignAllSignatureRequestsPanel = {
  render: () => {
    return (
      <WalletPanelStory {...storyContextProps}>
        <PendingSignatureRequestsPanel signMode='signAllTxs' />
      </WalletPanelStory>
    )
  }
}

export const _PendingSolanaTransactionSignaturesPanel = {
  render: () => {
    return (
      <WalletPanelStory {...storyContextProps}>
        <PendingSignatureRequestsPanel signMode='signTx' />
      </WalletPanelStory>
    )
  }
}

export default {
  parameters: {
    layout: 'centered'
  },
  component: PendingSignatureRequestsPanel
}
