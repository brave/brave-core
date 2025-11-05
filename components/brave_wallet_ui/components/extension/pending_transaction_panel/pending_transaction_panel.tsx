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
  CoinTypes,
} from '../../../constants/types'

// Components
import {
  ConfirmSwapTransaction, //
} from '../confirm_swap_transaction/confirm_swap_transaction'
import {
  ConfirmTransactionPanel, //
} from '../confirm-transaction-panel/confirm-transaction-panel'
import { LoadingPanel } from '../loading_panel/loading_panel'
import {
  EnableTransactionSimulations, //
  LoadingSimulation,
} from '../enable_transaction_simulations/enable_transaction_simulations'
import {
  ConfirmSimulatedTransactionPanel, //
} from '../confirm-transaction-panel/confirm_simulated_tx_panel'
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
  useGetEVMTransactionSimulationQuery,
  useGetHasTransactionSimulationSupportQuery,
  useGetIsTxSimulationOptInStatusQuery,
  useGetNetworkQuery,
  useGetSolanaTransactionSimulationQuery,
  useGetTransactionsQuery,
} from '../../../common/slices/api.slice'

// Style
import { LongWrapper } from '../../../stories/style'

/**
 * These are the types of transactions that
 * we can compute the expected results for locally.
 *
 * Skip Blowfish Simulation of these to reduce operating costs
 */
const SIMPLE_TRANSACTION_TYPES = [
  BraveWallet.TransactionType.ETHSend,
  BraveWallet.TransactionType.SolanaCompressedNftTransfer,
  BraveWallet.TransactionType.SolanaSystemTransfer,
  BraveWallet.TransactionType.ETHSwap, // use safer-sign UI
]

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

  const shouldSkipSimulation = SIMPLE_TRANSACTION_TYPES.includes(
    selectedPendingTransaction.txType,
  )

  const {
    data: txSimulationOptIn, //
    isLoading: isLoadingSimulationOptInStatus,
  } = useGetIsTxSimulationOptInStatusQuery(
    shouldSkipSimulation ? skipToken : undefined,
  )

  const isSimulationPermitted =
    !shouldSkipSimulation
    && txSimulationOptIn === BraveWallet.BlowfishOptInStatus.kAllowed

  const {
    data: networkHasTxSimulationSupport,
    isLoading: isCheckingSimulationNetworkSupport,
  } = useGetHasTransactionSimulationSupportQuery(
    shouldSkipSimulation
      ? skipToken
      : {
          chainId: selectedPendingTransaction.chainId,
          coin: selectedPendingTxCoinType,
        },
  )

  const {
    data: evmTxSimulation,
    isLoading: isLoadingEvmTxSimulation,
    isFetching: isFetchingEvmTxSimulation,
    isError: hasEvmSimulationError,
    refetch: retryEvmSimulation,
  } = useGetEVMTransactionSimulationQuery(
    isSimulationPermitted
      && networkHasTxSimulationSupport
      && selectedPendingTxCoinType === CoinTypes.ETH
      ? {
          txMetaId: selectedPendingTransaction.id,
        }
      : skipToken,
  )
  const {
    data: solanaTxSimulation,
    isLoading: isLoadingSolanaTxSimulation,
    isFetching: isFetchingSolanaTxSimulation,
    isError: hasSolanaSimulationError,
    refetch: retrySolanaSimulation,
  } = useGetSolanaTransactionSimulationQuery(
    isSimulationPermitted
      && networkHasTxSimulationSupport
      && selectedPendingTxCoinType === CoinTypes.SOL
      ? {
          txMetaId: selectedPendingTransaction.id,
        }
      : skipToken,
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

  // render

  // Simulations Opt-in screen
  if (
    networkHasTxSimulationSupport
    && txSimulationOptIn === BraveWallet.BlowfishOptInStatus.kUnset
    && selectedPendingTransaction.txType !== BraveWallet.TransactionType.ETHSwap
  ) {
    return <EnableTransactionSimulations />
  }

  // Loading
  if (isLoadingSimulationOptInStatus || isCheckingSimulationNetworkSupport) {
    return <LoadingPanel />
  }

  // Simulating
  if (
    isSimulationPermitted
    && networkHasTxSimulationSupport
    && (isLoadingEvmTxSimulation
      || isFetchingEvmTxSimulation
      || isFetchingSolanaTxSimulation
      || isLoadingSolanaTxSimulation)
  ) {
    return (
      <LongWrapper padding='0px'>
        <LoadingSimulation />
      </LongWrapper>
    )
  }

  // Simulated EVM Transaction
  if (
    isSimulationPermitted
    && networkHasTxSimulationSupport
    // has valid EVM pending transaction simulation
    && selectedPendingTxCoinType === CoinTypes.ETH
    && !hasEvmSimulationError
    && !isLoadingEvmTxSimulation
    && !isFetchingEvmTxSimulation
    && evmTxSimulation
  ) {
    return (
      <LongWrapper>
        <ConfirmSimulatedTransactionPanel
          key={'EVM'}
          simulationType={'EVM'}
          txSimulation={evmTxSimulation}
        />
      </LongWrapper>
    )
  }

  // Simulated Solana Transaction
  if (
    isSimulationPermitted
    && networkHasTxSimulationSupport
    && selectedPendingTxCoinType === CoinTypes.SOL
    && !hasSolanaSimulationError
    && !isLoadingSolanaTxSimulation
    && !isFetchingSolanaTxSimulation
    && solanaTxSimulation
  ) {
    return (
      <LongWrapper>
        <ConfirmSimulatedTransactionPanel
          key={'SVM'}
          simulationType={'SVM'}
          txSimulation={solanaTxSimulation}
        />
      </LongWrapper>
    )
  }

  // Safer-Sign (Brave Swap)
  if (
    selectedPendingTransaction.txType === BraveWallet.TransactionType.ETHSwap
  ) {
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
    selectedPendingTransaction.txType === BraveWallet.TransactionType.ETHSend
    || selectedPendingTransaction.txType
      === BraveWallet.TransactionType.ERC20Transfer
    || selectedPendingTransaction.txType
      === BraveWallet.TransactionType.SolanaSystemTransfer
    || selectedPendingTransaction.txType
      === BraveWallet.TransactionType.SolanaSPLTokenTransfer
    || selectedPendingTransaction.txType
      === BraveWallet.TransactionType
        .SolanaSPLTokenTransferWithAssociatedTokenAccountCreation
    || selectedPendingTransaction.txType === BraveWallet.TransactionType.Other
  ) {
    return <ConfirmSendTransaction />
  }

  // Defaults
  return (
    <ConfirmTransactionPanel
      data-testid='confirm-tx-panel'
      showSimulationNotSupportedMessage={
        isSimulationPermitted && !networkHasTxSimulationSupport
      }
      retrySimulation={
        isSimulationPermitted && networkHasTxSimulationSupport
          ? selectedPendingTxCoinType === CoinTypes.SOL
            && hasSolanaSimulationError
            ? retrySolanaSimulation
            : hasEvmSimulationError
              ? retryEvmSimulation
              : undefined
          : undefined
      }
    />
  )
}
