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
  CoinTypes
} from '../../../constants/types'

// Components
import { ConfirmSwapTransaction } from '../confirm-transaction-panel/swap'
import {
  ConfirmTransactionPanel //
} from '../confirm-transaction-panel/confirm-transaction-panel'
import { LoadingPanel } from '../loading_panel/loading_panel'
import {
  EnableTransactionSimulations, //
  LoadingSimulation
} from '../enable_transaction_simulations/enable_transaction_simulations'
import {
  ConfirmSimulatedTransactionPanel //
} from '../confirm-transaction-panel/confirm_simulated_tx_panel'

// Utils
import { getCoinFromTxDataUnion } from '../../../utils/network-utils'

import {
  useGetEVMTransactionSimulationQuery,
  useGetHasTransactionSimulationSupportQuery,
  useGetIsTxSimulationOptInStatusQuery,
  useGetSolanaTransactionSimulationQuery
} from '../../../common/slices/api.slice'

// Style
import { LongWrapper } from '../../../stories/style'

interface Props {
  selectedPendingTransaction: SerializableTransactionInfo
}

export const PendingTransactionPanel: React.FC<Props> = ({
  selectedPendingTransaction
}) => {
  // queries & query args
  const selectedPendingTxCoinType = getCoinFromTxDataUnion(
    selectedPendingTransaction.txDataUnion
  )

  const {
    data: txSimulationOptIn, //
    isLoading: isLoadingSimulationOptInStatus
  } = useGetIsTxSimulationOptInStatusQuery()
  const isSimulationPermitted =
    txSimulationOptIn === BraveWallet.BlowfishOptInStatus.kAllowed

  const {
    data: networkHasTxSimulationSupport,
    isLoading: isCheckingSimulationNetworkSupport
  } = useGetHasTransactionSimulationSupportQuery({
    chainId: selectedPendingTransaction.chainId,
    coinType: selectedPendingTxCoinType
  })

  const {
    data: evmTxSimulation,
    isLoading: isLoadingEvmTxSimulation,
    isFetching: isFetchingEvmTxSimulation,
    isError: hasEvmSimulationError,
    refetch: retryEvmSimulation
  } = useGetEVMTransactionSimulationQuery(
    isSimulationPermitted &&
      networkHasTxSimulationSupport &&
      selectedPendingTxCoinType === CoinTypes.ETH &&
      // skip simulating in favor of "safer-sign" UI for Brave and COW Swaps
      selectedPendingTransaction.txType !== BraveWallet.TransactionType.ETHSwap
      ? {
          chainId: selectedPendingTransaction.chainId,
          coinType: selectedPendingTxCoinType,
          id: selectedPendingTransaction.id
        }
      : skipToken
  )
  const {
    data: solanaTxSimulation,
    isLoading: isLoadingSolanaTxSimulation,
    isFetching: isFetchingSolanaTxSimulation,
    isError: hasSolanaSimulationError,
    refetch: retrySolanaSimulation
  } = useGetSolanaTransactionSimulationQuery(
    isSimulationPermitted &&
      networkHasTxSimulationSupport &&
      selectedPendingTxCoinType === CoinTypes.SOL
      ? {
          chainId: selectedPendingTransaction.chainId,
          id: selectedPendingTransaction.id,
          mode: 'transactionInfo'
        }
      : skipToken
  )

  // render

  // Simulations Opt-in screen
  if (
    networkHasTxSimulationSupport &&
    txSimulationOptIn === BraveWallet.BlowfishOptInStatus.kUnset &&
    selectedPendingTransaction.txType !== BraveWallet.TransactionType.ETHSwap
  ) {
    return <EnableTransactionSimulations />
  }

  // Loading
  if (isLoadingSimulationOptInStatus || isCheckingSimulationNetworkSupport) {
    return <LoadingPanel />
  }

  // Simulating
  if (
    isSimulationPermitted &&
    networkHasTxSimulationSupport &&
    (isLoadingEvmTxSimulation ||
      isFetchingEvmTxSimulation ||
      isFetchingSolanaTxSimulation ||
      isLoadingSolanaTxSimulation)
  ) {
    return <LoadingSimulation />
  }

  // Simulated EVM Transaction
  if (
    isSimulationPermitted &&
    networkHasTxSimulationSupport &&
    // has valid EVM pending transaction simulation
    selectedPendingTxCoinType === CoinTypes.ETH &&
    !hasEvmSimulationError &&
    !isLoadingEvmTxSimulation &&
    !isFetchingEvmTxSimulation &&
    evmTxSimulation
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
    isSimulationPermitted &&
    networkHasTxSimulationSupport &&
    selectedPendingTxCoinType === CoinTypes.SOL &&
    !hasSolanaSimulationError &&
    !isLoadingSolanaTxSimulation &&
    !isFetchingSolanaTxSimulation &&
    solanaTxSimulation
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
    return (
      <LongWrapper>
        <ConfirmSwapTransaction />
      </LongWrapper>
    )
  }

  // Defaults
  return (
    <LongWrapper data-testid='confirm-tx-panel'>
      <ConfirmTransactionPanel
        showSimulationNotSupportedMessage={
          isSimulationPermitted && !networkHasTxSimulationSupport
        }
        retrySimulation={
          isSimulationPermitted && networkHasTxSimulationSupport
            ? selectedPendingTxCoinType === CoinTypes.SOL &&
              hasSolanaSimulationError
              ? retrySolanaSimulation
              : hasEvmSimulationError
              ? retryEvmSimulation
              : undefined
            : undefined
        }
      />
    </LongWrapper>
  )
}
