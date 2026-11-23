// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { BraveWallet } from '../../../constants/types'
import { mockEthAccount } from '../../../common/constants/mocks'
import {
  mockETHNativeTokenSendTransaction, //
} from '../../../stories/mock-data/mock-transaction-info'
import {
  mockUniswapOriginInfo, //
} from '../../../stories/mock-data/mock-origin-info'
import {
  deserializeTransaction, //
} from '../../../utils/model-serialization-utils'

// Components
import { ConfirmSendTransaction } from './confirm_send_transaction'
import {
  WalletPanelStory, //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'

const mockNftContractAddress = '0xabcdefabcdefabcdefabcdefabcdefabcdefabcd'

const mockSetApprovalForAllTransaction = {
  ...mockETHNativeTokenSendTransaction,
  id: 'set-approval-for-all-tx',
  dappOriginInfo: mockUniswapOriginInfo,
  txType: BraveWallet.TransactionType.ERC721SetApprovalForAll,
  txArgs: [mockEthAccount.accountId.address, '0x1'],
  txParams: ['address', 'bool'],
  effectiveRecipient: mockNftContractAddress,
  txDataUnion: {
    ethTxData1559: {
      baseData: {
        chainId: '0x1',
        nonce: '',
        gasPrice: '0x5f5e100',
        gasLimit: '0x5208',
        to: mockNftContractAddress,
        value: '0x0',
        data: [],
        signOnly: false,
        signedTransaction: undefined,
      },
      maxPriorityFeePerGas: '',
      maxFeePerGas: '',
    },
  },
}

export const _ConfirmSendTransaction = {
  render: () => {
    return (
      <WalletPanelStory
        uiStateOverride={{
          selectedPendingTransactionId: mockETHNativeTokenSendTransaction.id,
        }}
      >
        <ConfirmSendTransaction />
      </WalletPanelStory>
    )
  },
}

export const _ConfirmSetApprovalForAll = {
  render: () => {
    return (
      <WalletPanelStory
        uiStateOverride={{
          selectedPendingTransactionId: mockSetApprovalForAllTransaction.id,
        }}
        walletApiDataOverrides={{
          transactionInfos: [
            deserializeTransaction(mockSetApprovalForAllTransaction),
          ],
        }}
      >
        <ConfirmSendTransaction />
      </WalletPanelStory>
    )
  },
}

export default {
  title: 'Wallet/Panel/Panels/Confirm Send Transaction Panel',
  component: ConfirmSendTransaction,
  parameters: {
    layout: 'centered',
  },
}
