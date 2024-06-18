// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// types
import { BraveWallet } from '../../../constants/types'

// mocks
import {
  mockSolDappSignAllTransactionsRequest,
  mockSolDappSignAndSendTransactionRequest,
  mockSolDappSignTransactionRequest
} from '../../../common/constants/mocks'
import {
  mockSolanaMainnetNetwork //
} from '../../../stories/mock-data/mock-networks'
import {
  mockSignMessageRequest //
} from '../../../stories/mock-data/mock-eth-requests'

// utils
import { SignInWithEthereumError } from './sign_in_with_ethereum_error'
import {
  deserializeTransaction //
} from '../../../utils/model-serialization-utils'

// components
import {
  WalletPanelStory //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'
import { SignTransactionPanel } from './sign-transaction-panel'
import { SignInWithEthereum } from './sign_in_with_ethereum'

export const _SignAllSolanaTxPanel = {
  render: () => {
    return (
      <WalletPanelStory
        uiStateOverride={{
          selectedPendingTransactionId:
            mockSolDappSignAndSendTransactionRequest.id
        }}
        walletApiDataOverrides={{
          signTransactionRequests: [mockSolDappSignTransactionRequest],
          signAllTransactionsRequests: [mockSolDappSignAllTransactionsRequest],
          transactionInfos: [
            deserializeTransaction({
              ...mockSolDappSignAndSendTransactionRequest,
              txStatus: BraveWallet.TransactionStatus.Unapproved
            })
          ]
        }}
      >
        <SignTransactionPanel
          signMode={'signAllTxs'}
          isSigningDisabled={false}
          network={mockSolanaMainnetNetwork}
          queueNextSignTransaction={function (): void {
            throw new Error('Function not implemented.')
          }}
          selectedQueueData={mockSolDappSignAllTransactionsRequest}
          queueLength={1}
          queueNumber={0}
        />
      </WalletPanelStory>
    )
  }
}

export const _SignSolanaTxPanel = {
  render: () => {
    return (
      <WalletPanelStory
        walletApiDataOverrides={{
          signTransactionRequests: [mockSolDappSignTransactionRequest],
          signAllTransactionsRequests: [mockSolDappSignAllTransactionsRequest],
          transactionInfos: [
            deserializeTransaction(mockSolDappSignAndSendTransactionRequest)
          ]
        }}
      >
        <SignTransactionPanel
          signMode='signTx'
          isSigningDisabled={false}
          network={mockSolanaMainnetNetwork}
          queueNextSignTransaction={function (): void {
            throw new Error('Function not implemented.')
          }}
          selectedQueueData={mockSolDappSignTransactionRequest}
          queueLength={1}
          queueNumber={0}
        />
      </WalletPanelStory>
    )
  }
}

export const _SignInWithEthereumErrorPanel = {
  render: () => {
    return (
      <WalletPanelStory>
        <SignInWithEthereumError />
      </WalletPanelStory>
    )
  }
}

export const _SignInWithEthereumPanel = {
  render: () => {
    return (
      <WalletPanelStory>
        <SignInWithEthereum
          data={mockSignMessageRequest}
          onCancel={() => { }}
          onSignIn={() => { }}
        />
      </WalletPanelStory>
    )
  }
}

export default {
  component: SignInWithEthereum,
  parameters: {
    layout: 'centered'
  }
}
