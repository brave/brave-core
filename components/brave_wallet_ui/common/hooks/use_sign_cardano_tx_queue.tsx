// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// types
import { BraveWallet } from '../../constants/types'

// hooks
import {
  useGetPendingSignCardanoTransactionRequestsQuery,
  useProcessSignCardanoTransactionRequestMutation,
} from '../slices/api.slice'
import { useAccountQuery } from '../slices/api.slice.extra'

export interface UseProcessCardanoTxProps {
  request: BraveWallet.SignCardanoTransactionRequest
}

export const useProcessSignCardanoTransaction = (
  props: UseProcessCardanoTxProps,
) => {
  // mutations
  const [processSignCardanoTransactionRequest] =
    useProcessSignCardanoTransactionRequestMutation()

  // methods
  const cancelSign = React.useCallback(async () => {
    await processSignCardanoTransactionRequest({
      approved: false,
      id: props.request.id,
      error: null,
    }).unwrap()
  }, [props, processSignCardanoTransactionRequest])

  const sign = React.useCallback(async () => {
    await processSignCardanoTransactionRequest({
      approved: true,
      id: props.request.id,
      error: null,
    }).unwrap()
  }, [processSignCardanoTransactionRequest, props])

  // render
  return {
    cancelSign,
    sign,
  }
}

export const useSignCardanoTransactionsQueue = () => {
  // state
  const [queueNumber, setQueueNumber] = React.useState<number>(1)
  const queueIndex = queueNumber - 1

  // queries
  const { data: signCardanoTransactionRequests } =
    useGetPendingSignCardanoTransactionRequestsQuery()
  const selectedRequest = signCardanoTransactionRequests
    ? signCardanoTransactionRequests.at(queueIndex)
    : undefined
  const { account } = useAccountQuery(selectedRequest?.accountId)

  // computed
  const queueLength = signCardanoTransactionRequests?.length || 0

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
