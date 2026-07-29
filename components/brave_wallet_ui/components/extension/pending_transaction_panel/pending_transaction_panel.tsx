// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Types
import {
  BraveWallet,
  SerializableTransactionInfo,
} from '../../../constants/types'

// Components
import {
  ConfirmSwapTransaction, //
} from '../confirm_swap_transaction/confirm_swap_transaction'
import {
  ConfirmTransactionPanel, //
} from '../confirm-transaction-panel/confirm-transaction-panel'
import { AllowSpendPanel } from '../allow_spend_panel/allow_spend_panel'
import {
  ConfirmSendTransaction, //
} from '../confirm_send_transaction/confirm_send_transaction'
import {
  CancelSpeedupTransaction, //
} from '../cancel_speedup_transaction/cancel_speedup_transaction'

// Utils
import { getCoinFromTxDataUnion } from '../../../utils/network-utils'
import { isCancelTransaction } from '../../../utils/tx-utils'

import {
  useGetNetworkQuery,
  useGetTransactionsQuery,
} from '../../../common/slices/api.slice'

interface Props {
  selectedPendingTransaction: SerializableTransactionInfo
}

export const PendingTransactionPanel: React.FC<Props> = ({
  selectedPendingTransaction,
}) => {
  // queries & query args
  const selectedPendingTxCoinType = getCoinFromTxDataUnion(
    selectedPendingTransaction.txDataUnion,
  )

  const { data: selectedPendingTxNetwork } = useGetNetworkQuery(
    selectedPendingTransaction && selectedPendingTxCoinType
      ? {
          chainId: selectedPendingTransaction.chainId,
          coin: selectedPendingTxCoinType,
        }
      : skipToken,
  )

  const { data: submittedTransactions = [] } = useGetTransactionsQuery(
    selectedPendingTxNetwork && selectedPendingTxCoinType
      ? {
          accountId: null,
          chainId: selectedPendingTxNetwork.chainId,
          coinType: selectedPendingTxCoinType,
        }
      : skipToken,
  )

  // Detect if this is a cancel transaction
  const isCancelTx = isCancelTransaction(
    selectedPendingTransaction,
    submittedTransactions,
  )

  // Safer-Sign (Brave Swap)
  if (selectedPendingTransaction.swapInfo) {
    return <ConfirmSwapTransaction />
  }

  // Allow spend
  if (
    selectedPendingTransaction.txType
    === BraveWallet.TransactionType.ERC20Approve
  ) {
    return <AllowSpendPanel />
  }

  // Cancel
  if (isCancelTx) {
    return <CancelSpeedupTransaction />
  }

  // Send
  if (
    [
      BraveWallet.TransactionType.ETHSend,
      BraveWallet.TransactionType.ERC20Transfer,
      BraveWallet.TransactionType.SolanaSystemTransfer,
      BraveWallet.TransactionType.SolanaSPLTokenTransfer,
      BraveWallet.TransactionType
        .SolanaSPLTokenTransferWithAssociatedTokenAccountCreation,
      BraveWallet.TransactionType.Other,
      BraveWallet.TransactionType.CardanoSendLovelace,
      BraveWallet.TransactionType.CardanoSendToken,
    ].includes(selectedPendingTransaction.txType)
  ) {
    return <ConfirmSendTransaction />
  }

  // Defaults
  return <ConfirmTransactionPanel data-testid='confirm-tx-panel' />
}
