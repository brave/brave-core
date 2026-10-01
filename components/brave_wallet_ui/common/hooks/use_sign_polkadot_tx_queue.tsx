// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// types
import { BraveWallet } from '../../constants/types'

// hooks
import {
  useGetPendingSignPolkadotTransactionRequestsQuery,
  useProcessSignPolkadotTransactionRequestMutation,
} from '../slices/api.slice'
import { useAccountQuery } from '../slices/api.slice.extra'

export interface UseProcessPolkadotTxProps {
  request: BraveWallet.SignPolkadotTransactionRequest
}

export const useProcessSignPolkadotTransaction = (
  props: UseProcessPolkadotTxProps,
) => {
  // mutations
  const [processSignPolkadotTransactionRequest] =
    useProcessSignPolkadotTransactionRequestMutation()

  // methods
  const cancelSign = React.useCallback(async () => {
    await processSignPolkadotTransactionRequest({
      approved: false,
      id: props.request.id,
      error: null,
    }).unwrap()
  }, [props, processSignPolkadotTransactionRequest])

  const sign = React.useCallback(async () => {
    await processSignPolkadotTransactionRequest({
      approved: true,
      id: props.request.id,
      error: null,
    }).unwrap()
  }, [processSignPolkadotTransactionRequest, props])

  // render
  return {
    cancelSign,
    sign,
  }
}

export const useSignPolkadotTransactionsQueue = () => {
  // state
  const [queueNumber, setQueueNumber] = React.useState<number>(1)
  const queueIndex = queueNumber - 1

  // queries
  const { data: signPolkadotTransactionRequests } =
    useGetPendingSignPolkadotTransactionRequestsQuery()
  const selectedRequest = signPolkadotTransactionRequests
    ? signPolkadotTransactionRequests.at(queueIndex)
    : undefined
  const { account } = useAccountQuery(selectedRequest?.accountId)

  // computed
  const queueLength = signPolkadotTransactionRequests?.length || 0

  // force signing messages in-order
  const isDisabled = queueNumber !== 1

  // methods
  const queueNextSignTransaction = React.useCallback(() => {
    setQueueNumber((prev) => (prev === queueLength ? 1 : prev + 1))
  }, [queueLength])

  // render
  return {
    selectedRequest,
    isDisabled,
    signingAccount: account,
    queueNextSignTransaction,
    queueLength,
    queueNumber,
    queueIndex,
  }
}
