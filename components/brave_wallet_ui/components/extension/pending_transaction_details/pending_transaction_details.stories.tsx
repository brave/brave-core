// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Mock Data
import {
  mockEthSendTransaction, //
  mockBtcSendTransaction,
  mockZecSendTransaction,
  mockERC20TransferTransaction,
  mockedErc20ApprovalTransaction,
  mockSOLTXInstructions,
} from '../../../stories/mock-data/mock-transaction-info'

// Utils
import { getLocale } from '../../../../common/locale'
import {
  getTypedSolanaTxInstructions, //
} from '../../../utils/solana-instruction-utils'

// Components
import {
  PendingTransactionDetails, //
} from './pending_transaction_details'
import {
  WalletPanelStory, //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'
import { BottomSheet } from '../../shared/bottom_sheet/bottom_sheet'

type StorybookPendingTxDetailsTypes =
  | 'BTC Send'
  | 'ETH Send'
  | 'ZEC Send'
  | 'FIL Send'
  | 'ERC20 Transfer'
  | 'ERC20 Approval'
  | 'SOL Send'

const StorybookPendingTxArgs: StorybookPendingTxDetailsTypes[] = [
  'BTC Send',
  'ETH Send',
  'ZEC Send',
  'FIL Send',
  'ERC20 Transfer',
  'ERC20 Approval',
  'SOL Send',
]

type StorybookTransactionArgs = {
  transactionType: StorybookPendingTxDetailsTypes
}

export const _PendingTransactionDetails = {
  render: (args: StorybookTransactionArgs) => {
    const { transactionType } = args

    let transactionInfo
    switch (transactionType) {
      case 'BTC Send':
        transactionInfo = mockBtcSendTransaction
        break
      case 'ZEC Send':
        transactionInfo = mockZecSendTransaction
        break
      case 'ERC20 Transfer':
        transactionInfo = mockERC20TransferTransaction
        break
      case 'ERC20 Approval':
        transactionInfo = mockedErc20ApprovalTransaction
        break
      case 'SOL Send':
        transactionInfo = mockSOLTXInstructions
        break
      case 'FIL Send':
      case 'ETH Send':
      default:
        transactionInfo = mockEthSendTransaction
        break
    }

    return (
      <BottomSheet
        isOpen={true}
        title={getLocale('braveWalletDetails')}
        onClose={() => alert('Close Clicked')}
      >
        <PendingTransactionDetails
          transactionInfo={transactionInfo}
          instructions={
            transactionType === 'SOL Send'
              ? getTypedSolanaTxInstructions(
                  mockSOLTXInstructions.txDataUnion.solanaTxData,
                )
              : undefined
          }
        />
      </BottomSheet>
    )
  },
}

export default {
  title: 'Wallet/Panel/Components/Pending Transaction Details',
  component: PendingTransactionDetails,
  parameters: {
    layout: 'centered',
  },
  argTypes: {
    transactionType: {
      options: StorybookPendingTxArgs,
      control: { type: 'select' },
    },
  },
  decorators: [
    (Story: any) => (
      <WalletPanelStory>
        <Story />
      </WalletPanelStory>
    ),
  ],
}
