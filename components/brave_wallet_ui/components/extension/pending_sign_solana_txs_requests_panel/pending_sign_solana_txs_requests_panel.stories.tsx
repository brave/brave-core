// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// types
import { BraveWallet } from '../../../constants/types'

// mocks
import {
  mockSolanaAccount,
  mockSolDappSignAllTransactionsRequest,
  mockSolDappSignAndSendTransactionRequest,
  mockSvmSimulationResult,
} from '../../../common/constants/mocks'

// utils
import {
  deserializeTransaction, //
} from '../../../utils/model-serialization-utils'

// components
import {
  WalletPanelStory, //
  WalletPanelStoryProps,
} from '../../../stories/wrappers/wallet-panel-story-wrapper'

import PendingSignSolanaTransactionsRequestsPanel from './pending_sign_solana_txs_requests_panel'

const storyContextProps: WalletPanelStoryProps = {
  walletApiDataOverrides: {
    accountInfos: [mockSolanaAccount],
    selectedAccountId: mockSolanaAccount.accountId,
    signSolTransactionsRequests: [
      mockSolDappSignAllTransactionsRequest,
      mockSolDappSignAllTransactionsRequest,
    ],
    transactionInfos: [
      deserializeTransaction(mockSolDappSignAndSendTransactionRequest),
      deserializeTransaction({
        ...mockSolDappSignAndSendTransactionRequest,
        txStatus: BraveWallet.TransactionStatus.Unapproved,
      }),
    ],
    svmSimulationResponse: mockSvmSimulationResult,
  },
  uiStateOverride: {
    selectedPendingTransactionId: mockSolDappSignAndSendTransactionRequest.id,
  },
}

export const _PendingSignSolanaTransactionsRequestsPanel = {
  render: () => {
    return (
      <WalletPanelStory {...storyContextProps}>
        <PendingSignSolanaTransactionsRequestsPanel />
      </WalletPanelStory>
    )
  },
}

export default {
  parameters: {
    layout: 'centered',
  },
  title: 'Wallet/Panel/Panels/Sign Transaction',
  component: PendingSignSolanaTransactionsRequestsPanel,
}
