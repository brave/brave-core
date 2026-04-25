// Copyright (c) 2022 The Brave Authors. All rights reserved.
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
} from '../../../common/constants/mocks'
import {
  mockSolanaMainnetNetwork, //
} from '../../../stories/mock-data/mock-networks'
import {
  deserializeTransaction, //
} from '../../../utils/model-serialization-utils'
// components
import {
  WalletPanelStory, //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'
import { SignSolanaTxsPanel } from './sign_solana_txs_panel'

export const _SignAllSolanaTxPanel = {
  render: () => {
    return (
      <WalletPanelStory
        uiStateOverride={{
          selectedPendingTransactionId:
            mockSolDappSignAndSendTransactionRequest.id,
        }}
        walletApiDataOverrides={{
          signSolTransactionsRequests: [mockSolDappSignAllTransactionsRequest],
          transactionInfos: [
            deserializeTransaction({
              ...mockSolDappSignAndSendTransactionRequest,
              txStatus: BraveWallet.TransactionStatus.Unapproved,
            }),
          ],
        }}
      >
        <SignSolanaTxsPanel
          isSigningDisabled={false}
          network={mockSolanaMainnetNetwork}
          queueNextSignTransaction={function (): void {
            throw new Error('Function not implemented.')
          }}
          selectedRequest={mockSolDappSignAllTransactionsRequest}
          signingAccount={mockSolanaAccount}
          queueLength={1}
          queueNumber={0}
        />
      </WalletPanelStory>
    )
  },
}

export default {
  title: 'Wallet/Panel/Panels/Sign Transaction',
  parameters: {
    layout: 'centered',
  },
}
