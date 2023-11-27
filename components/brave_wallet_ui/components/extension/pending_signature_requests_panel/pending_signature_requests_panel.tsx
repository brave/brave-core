// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Hooks
import {
  useSignSolanaTransactionsQueue //
} from '../../../common/hooks/use_sign_solana_tx_queue'
import {
  useGetIsTxSimulationOptInStatusQuery,
  useGetSolanaTransactionSimulationQuery
} from '../../../common/slices/api.slice'

// Components
import SignTransactionPanel from '../sign-panel/sign-transaction-panel'
import { LoadingPanel } from '../loading_panel/loading_panel'
import {
  EnableTransactionSimulations, //
  LoadingSimulation
} from '../enable_transaction_simulations/enable_transaction_simulations'
import {
  SignSimulatedTransactionPanel //
} from '../confirm-transaction-panel/sign_simulated_tx_panel'

interface Props {
  signMode: 'signTx' | 'signAllTxs'
}

export const PendingSignatureRequestsPanel: React.FC<Props> = ({
  signMode
}) => {
  // custom hooks
  const {
    isDisabled,
    network,
    queueLength,
    queueNextSignTransaction,
    queueNumber,
    selectedQueueData,
    signingAccount
  } = useSignSolanaTransactionsQueue(signMode)

  // queries
  const { data: txSimulationOptIn } = useGetIsTxSimulationOptInStatusQuery()
  const {
    data: solanaSimulation,
    isLoading: isLoadingSolanaSimulation,
    isFetching: isFetchingSolanaSimulation,
    isError: hasSimulationError
  } = useGetSolanaTransactionSimulationQuery(
    txSimulationOptIn === 'allowed' &&
      selectedQueueData?.id !== undefined &&
      network
      ? {
          chainId: network.chainId,
          id: selectedQueueData.id,
          mode:
            signMode === 'signAllTxs'
              ? 'signAllTransactionsRequest'
              : 'signTransactionRequest'
        }
      : skipToken
  )

  // render

  // Simulations Opt-in screen
  if (txSimulationOptIn === 'unset') {
    return <EnableTransactionSimulations />
  }

  // Loading/Fetching network
  if (!network) {
    return <LoadingPanel />
  }

  // Loading/Fetching Simulation or network
  if (
    txSimulationOptIn === 'allowed' &&
    (isLoadingSolanaSimulation || isFetchingSolanaSimulation)
  ) {
    return <LoadingSimulation />
  }

  // Simulated Signature Request
  if (
    txSimulationOptIn === 'allowed' &&
    !hasSimulationError &&
    solanaSimulation
  ) {
    return (
      <SignSimulatedTransactionPanel
        signMode={signMode}
        key={'SVM'}
        txSimulation={solanaSimulation}
        isSigningDisabled={isDisabled}
        network={network}
        queueLength={queueLength}
        queueNextSignTransaction={queueNextSignTransaction}
        queueNumber={queueNumber}
        selectedQueueData={selectedQueueData}
        signingAccount={signingAccount}
      />
    )
  }

  // Default (not simulated)
  return (
    <SignTransactionPanel
      signMode={signMode}
      isSigningDisabled={isDisabled}
      network={network}
      queueLength={queueLength}
      queueNextSignTransaction={queueNextSignTransaction}
      queueNumber={queueNumber}
      selectedQueueData={selectedQueueData}
      signingAccount={signingAccount}
    />
  )
}

export default PendingSignatureRequestsPanel
