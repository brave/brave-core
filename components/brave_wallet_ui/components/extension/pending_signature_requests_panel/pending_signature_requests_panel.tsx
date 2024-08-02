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
  useSignSolanaTransactionsQueue //
} from '../../../common/hooks/use_sign_solana_tx_queue'
import {
  useGetHasTransactionSimulationSupportQuery,
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

// Style
import { LongWrapper } from '../../../stories/style'

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
  const {
    data: txSimulationOptIn, //
    isLoading: isLoadingSimulationOptInStatus
  } = useGetIsTxSimulationOptInStatusQuery()
  const isSimulationPermitted =
    txSimulationOptIn === BraveWallet.BlowfishOptInStatus.kAllowed

  const {
    data: networkHasTxSimulationSupport,
    isLoading: isCheckingSimulationNetworkSupport
  } = useGetHasTransactionSimulationSupportQuery(
    network
      ? {
          chainId: network.chainId,
          coinType: network.coin
        }
      : skipToken
  )

  const {
    data: solanaSimulation,
    isLoading: isLoadingSolanaSimulation,
    isFetching: isFetchingSolanaSimulation,
    isError: hasSimulationError,
    refetch: retrySimulation
  } = useGetSolanaTransactionSimulationQuery(
    isSimulationPermitted &&
      network &&
      networkHasTxSimulationSupport &&
      selectedQueueData?.id !== undefined
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
  if (
    networkHasTxSimulationSupport &&
    txSimulationOptIn === BraveWallet.BlowfishOptInStatus.kUnset
  ) {
    return <EnableTransactionSimulations />
  }

  // Loading
  if (
    !network ||
    isLoadingSimulationOptInStatus ||
    isCheckingSimulationNetworkSupport
  ) {
    return <LoadingPanel />
  }

  // Simulating
  if (
    isSimulationPermitted &&
    (isLoadingSolanaSimulation || isFetchingSolanaSimulation)
  ) {
    return <LoadingSimulation />
  }

  // Simulated Signature Request
  if (
    isSimulationPermitted &&
    networkHasTxSimulationSupport &&
    !hasSimulationError &&
    solanaSimulation
  ) {
    return (
      <LongWrapper>
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
      </LongWrapper>
    )
  }

  // Default (not simulated)
  return (
    <LongWrapper>
      <SignTransactionPanel
        signMode={signMode}
        isSigningDisabled={isDisabled}
        network={network}
        queueLength={queueLength}
        queueNextSignTransaction={queueNextSignTransaction}
        queueNumber={queueNumber}
        selectedQueueData={selectedQueueData}
        signingAccount={signingAccount}
        showSimulationNotSupportedMessage={
          isSimulationPermitted && !networkHasTxSimulationSupport
        }
        retrySimulation={
          isSimulationPermitted &&
          networkHasTxSimulationSupport &&
          hasSimulationError
            ? retrySimulation
            : undefined
        }
      />
    </LongWrapper>
  )
}

export default PendingSignatureRequestsPanel
