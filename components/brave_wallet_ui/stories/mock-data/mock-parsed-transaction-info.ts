// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// utils
import { getCoinFromTxDataUnion } from '../../utils/network-utils'
import { parseTransactionWithPrices } from '../../utils/tx-utils'

// mocks
import { mockWalletState } from './mock-wallet-state'
import { mockedErc20ApprovalTransaction, mockTransactionInfo } from './mock-transaction-info'
import { mockNetworks } from './mock-networks'

export const mockParsedTransactionInfo = parseTransactionWithPrices({
  accounts: mockWalletState.accounts,
  tokensList: [
    ...mockWalletState.userVisibleTokensInfo,
    ...mockWalletState.fullTokenList
  ],
  spotPriceRegistry: {},
  tx: mockTransactionInfo,
  gasFee: mockWalletState.solFeeEstimates?.fee.toString() ?? '',
  transactionNetwork: mockNetworks.find(
    (n) =>
      n.chainId === mockTransactionInfo.chainId &&
      n.coin === getCoinFromTxDataUnion(mockTransactionInfo.txDataUnion)
  )
})

export const mockedParsedErc20ApprovalTransaction = parseTransactionWithPrices({
  accounts: mockWalletState.accounts,
  tokensList: [
    ...mockWalletState.userVisibleTokensInfo,
    ...mockWalletState.fullTokenList
  ],
  spotPriceRegistry: {},
  tx: mockedErc20ApprovalTransaction,
  gasFee: mockWalletState.solFeeEstimates?.fee.toString() ?? '',
  transactionNetwork: mockNetworks.find(
    (n) =>
      n.chainId === mockedErc20ApprovalTransaction.chainId &&
      n.coin ===
        getCoinFromTxDataUnion(mockedErc20ApprovalTransaction.txDataUnion)
  )
})
