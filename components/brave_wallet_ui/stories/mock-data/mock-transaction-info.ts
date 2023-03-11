// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// Types
import { BraveWallet, SerializableTransactionInfo } from '../../constants/types'

// Mocks
import { mockOriginInfo } from './mock-origin-info'

export const mockTransactionInfo: SerializableTransactionInfo = {
  chainId: '0x0',
  fromAddress: '0x7d66c9ddAED3115d93Bd1790332f3Cd06Cf52B14',
  id: '465a4d6646-kjlwf665',
  txArgs: ['0x0d8775f648430679a709e98d2b0cb6250d2887ef', '0x15ddf09c97b0000'],
  txDataUnion: {
    ethTxData1559: {
      baseData: {
        nonce: '0x1',
        gasPrice: '150',
        gasLimit: '21000',
        to: '2',
        value: '0x15ddf09c97b0000',
        data: Array.from(new Uint8Array(24)),
        signOnly: false,
        signedTransaction: undefined
      },
      chainId: '0x0',
      maxPriorityFeePerGas: '',
      maxFeePerGas: '',
      gasEstimation: undefined
    },
    ethTxData: undefined,
    solanaTxData: undefined,
    filTxData: undefined
  },
  txHash: '0xab834bab0000000000000000000000007be8076f4ea4a4ad08075c2508e481d6c946d12b00000000000000000000000073a29a1da971497',
  txStatus: 0,
  txParams: ['address', 'ammount'],
  txType: BraveWallet.TransactionType.ERC20Transfer,
  createdTime: { microseconds: 0 },
  submittedTime: { microseconds: 0 },
  confirmedTime: { microseconds: 0 },
  originInfo: mockOriginInfo,
  groupId: undefined
}

export const mockedErc20ApprovalTransaction = {
  ...mockTransactionInfo,
  txType: BraveWallet.TransactionType.ERC20Approve
}
