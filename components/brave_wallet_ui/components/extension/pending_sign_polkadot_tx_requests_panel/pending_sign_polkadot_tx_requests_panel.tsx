// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Hooks
import {
  useSignPolkadotTransactionsQueue, //
} from '../../../common/hooks/use_sign_polkadot_tx_queue'
import {
  useGetNetworkQuery,
  useGetPolkadotSignRequestDetailsQuery,
  useProcessSignPolkadotTransactionRequestMutation,
} from '../../../common/slices/api.slice'

// Components
import SignPolkadotTxPanel from '../sign-panel/sign_polkadot_tx_panel'
import { LoadingPanel } from '../loading_panel/loading_panel'

// Style
import { LongWrapper } from '../../../stories/style'

export const PendingSignPolkadotTransactionRequestsPanel: React.FC = () => {
  // custom hooks
  const {
    isDisabled,
    queueLength,
    queueNextSignTransaction,
    queueNumber,
    selectedRequest,
    signingAccount,
  } = useSignPolkadotTransactionsQueue()

  const { data: network } = useGetNetworkQuery(
    selectedRequest ? selectedRequest.chainId : skipToken,
  )

  // The description and fee are fetched here rather than gating the whole panel
  // on them, so the risk step stays interactive while the decode is in flight.
  const {
    data: details,
    isError: isDetailsError,
    isFetching: isFetchingDetails,
    isSuccess: isDetailsSuccess,
  } = useGetPolkadotSignRequestDetailsQuery(
    selectedRequest ? selectedRequest.id : skipToken,
  )

  // mutations
  const [processSignPolkadotTransactionRequest] =
    useProcessSignPolkadotTransactionRequestMutation()

  // computed
  // Settled with no description: the decode failed, so there is nothing the
  // user could approve.
  const isUndescribable =
    !isFetchingDetails && (isDetailsError || (isDetailsSuccess && !details))

  // effects
  // The Sign button can never ungate for a request nothing described, so reject
  // it rather than leaving the panel parked on it with the dapp still waiting.
  // Only the front of the queue can be processed, hence the isDisabled check: a
  // failure further back is rejected once it reaches the front.
  React.useEffect(() => {
    if (!selectedRequest || isDisabled || !isUndescribable) {
      return
    }

    processSignPolkadotTransactionRequest({
      approved: false,
      id: selectedRequest.id,
      error: 'failed to decode the signing request',
    })
  }, [
    isDisabled,
    isUndescribable,
    processSignPolkadotTransactionRequest,
    selectedRequest,
  ])

  // Loading
  // Undescribable requests are on their way out, so keep the dead-end panel off
  // screen rather than flashing it while the rejection lands.
  if (!network || !selectedRequest || !signingAccount || isUndescribable) {
    return <LoadingPanel />
  }

  return (
    <LongWrapper padding='0px'>
      <SignPolkadotTxPanel
        details={details ?? null}
        isFetchingDetails={isFetchingDetails}
        isSigningDisabled={isDisabled}
        network={network}
        queueLength={queueLength}
        queueNextSignTransaction={queueNextSignTransaction}
        queueNumber={queueNumber}
        selectedRequest={selectedRequest}
        signingAccount={signingAccount}
      />
    </LongWrapper>
  )
}

export default PendingSignPolkadotTransactionRequestsPanel
