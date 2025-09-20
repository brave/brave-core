// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Types
import { BraveWallet } from '../../../constants/types'

// Hooks
import {
  useSignSolanaTransactionsQueue, //
} from '../../../common/hooks/use_sign_solana_tx_queue'
import {
  useGetHasTransactionSimulationSupportQuery,
  useGetIsTxSimulationOptInStatusQuery,
  useGetNetworkQuery,
  useGetSolanaSignTransactionsRequestSimulationQuery,
} from '../../../common/slices/api.slice'

// Components
import SignSolanaTxsPanel from '../sign-panel/sign_solana_txs_panel'
import { LoadingPanel } from '../loading_panel/loading_panel'
import {
  EnableTransactionSimulations, //
  LoadingSimulation,
} from '../enable_transaction_simulations/enable_transaction_simulations'
import {
  SignSimulatedTransactionPanel, //
} from '../confirm-transaction-panel/sign_simulated_tx_panel'

// Style
import { LongWrapper } from '../../../stories/style'

export const PendingSignSolanaTransactionsRequestsPanel: React.FC = () => {
  // custom hooks
  const {
    isDisabled,
    queueLength,
    queueNextSignTransaction,
    queueNumber,
    selectedRequest,
    signingAccount,
  } = useSignSolanaTransactionsQueue()

  // queries
  const {
    data: txSimulationOptIn, //
    isLoading: isLoadingSimulationOptInStatus,
  } = useGetIsTxSimulationOptInStatusQuery()
  const isSimulationPermitted =
    txSimulationOptIn === BraveWallet.BlowfishOptInStatus.kAllowed

  const { data: network } = useGetNetworkQuery(
    selectedRequest ? selectedRequest.chainId : skipToken,
  )

  const {
    data: networkHasTxSimulationSupport,
    isLoading: isCheckingSimulationNetworkSupport,
  } = useGetHasTransactionSimulationSupportQuery(
    selectedRequest ? selectedRequest.chainId : skipToken,
  )

  const {
    data: solanaSimulation,
    isLoading: isLoadingSolanaSimulation,
    isFetching: isFetchingSolanaSimulation,
    isError: hasSimulationError,
    refetch: retrySimulation,
  } = useGetSolanaSignTransactionsRequestSimulationQuery(
    isSimulationPermitted && networkHasTxSimulationSupport && selectedRequest
      ? {
          signSolTransactionsRequestId: selectedRequest.id,
        }
      : skipToken,
  )

  // render

  // Simulations Opt-in screen
  if (
    networkHasTxSimulationSupport
    && txSimulationOptIn === BraveWallet.BlowfishOptInStatus.kUnset
  ) {
    return <EnableTransactionSimulations />
  }

  // Loading
  if (
    !network
    || !selectedRequest
    || !signingAccount
    || isLoadingSimulationOptInStatus
    || isCheckingSimulationNetworkSupport
  ) {
    return <LoadingPanel />
  }

  // Simulating
  if (
    isSimulationPermitted
    && (isLoadingSolanaSimulation || isFetchingSolanaSimulation)
  ) {
    return <LoadingSimulation />
  }

  // Simulated Signature Request
  if (
    isSimulationPermitted
    && networkHasTxSimulationSupport
    && !hasSimulationError
    && solanaSimulation
  ) {
    return (
      <LongWrapper>
        <SignSimulatedTransactionPanel
          key={'SVM'}
          txSimulation={solanaSimulation}
          isSigningDisabled={isDisabled}
          network={network}
          queueLength={queueLength}
          queueNextSignTransaction={queueNextSignTransaction}
          queueNumber={queueNumber}
          signSolTransactionsRequest={selectedRequest}
          signingAccount={signingAccount}
        />
      </LongWrapper>
    )
  }

  // Default (not simulated)
  return (
    <LongWrapper>
      <SignSolanaTxsPanel
        isSigningDisabled={isDisabled}
        network={network}
        queueLength={queueLength}
        queueNextSignTransaction={queueNextSignTransaction}
        queueNumber={queueNumber}
        selectedRequest={selectedRequest}
        signingAccount={signingAccount}
        showSimulationNotSupportedMessage={
          isSimulationPermitted && !networkHasTxSimulationSupport
        }
        retrySimulation={
          isSimulationPermitted
          && networkHasTxSimulationSupport
          && hasSimulationError
            ? retrySimulation
            : undefined
        }
      />
    </LongWrapper>
  )
}

export default PendingSignSolanaTransactionsRequestsPanel
