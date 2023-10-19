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
import { SignInWithEthereumError } from './sign_in_with_ethereum_error'
import {
  deserializeTransaction //
} from '../../../utils/model-serialization-utils'
import { BraveWallet } from '../../../constants/types'

export const _SignAllSolanaTxPanel = () => {
  return (
    <WalletPanelStory
      panelStateOverride={{
        selectedPanel: 'signTransaction',
        signTransactionRequests: [mockSolDappSignTransactionRequest],
        signAllTransactionsRequests: [mockSolDappSignAllTransactionsRequest]
      }}
      uiStateOverride={{
        selectedPendingTransactionId:
          mockSolDappSignAndSendTransactionRequest.id
      }}
      walletApiDataOverrides={{
        transactionInfos: [
          deserializeTransaction({
            ...mockSolDappSignAndSendTransactionRequest,
            txStatus: BraveWallet.TransactionStatus.Unapproved
          })
        ]
      }}
    >
      <SignTransactionPanel signMode={'signAllTxs'} />
    </WalletPanelStory>
  )
}

_SignAllSolanaTxPanel.story = {
  name: 'Sign Solana All Transactions Panel'
}

export const _SignSolanaTxPanel = () => {
  return (
    <WalletPanelStory
      panelStateOverride={{
        selectedPanel: 'signTransaction',
        signTransactionRequests: [mockSolDappSignTransactionRequest],
        signAllTransactionsRequests: [mockSolDappSignAllTransactionsRequest]
      }}
      walletApiDataOverrides={{
        transactionInfos: [
          deserializeTransaction(mockSolDappSignAndSendTransactionRequest)
        ]
      }}
    >
      <SignTransactionPanel signMode='signTx' />
    </WalletPanelStory>
  )
}

_SignSolanaTxPanel.story = {
  name: 'Sign Solana Transaction Panel'
}

export const _SignInWithEthereumError = () => {
  return (
    <WalletPanelStory>
      <SignInWithEthereumError />
    </WalletPanelStory>
  )
}

_SignInWithEthereumError.story = {
  name: 'Sign in with Ethereum Error Panel'
}

export default {
  parameters: {
    layout: 'centered'
  }
}
