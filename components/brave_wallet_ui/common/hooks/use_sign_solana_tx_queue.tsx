// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// types
import { BraveWallet } from '../../constants/types'

// hooks
import {
  useGetPendingSignSolTransactionsRequestsQuery,
  useProcessSignSolTransactionsRequestHardwareMutation,
  useProcessSignSolTransactionsRequestMutation,
} from '../slices/api.slice'
import { useAccountQuery } from '../slices/api.slice.extra'

export interface UseProcessSolTxProps {
  request: BraveWallet.SignSolTransactionsRequest
}

export const useProcessSignSolanaTransaction = (
  props: UseProcessSolTxProps,
) => {
  // mutations
  const [processSignSolTransactionsRequest] =
    useProcessSignSolTransactionsRequestMutation()
  const [processSignSolTransactionsRequestHardware] =
    useProcessSignSolTransactionsRequestHardwareMutation()

  // methods
  const cancelSign = React.useCallback(async () => {
    await processSignSolTransactionsRequest({
      approved: false,
      id: props.request.id,
      hwSignatures: [],
      error: null,
    }).unwrap()
  }, [props, processSignSolTransactionsRequest])

  const sign = React.useCallback(async () => {
    const isHwAccount =
      props.request.fromAccountId.kind === BraveWallet.AccountKind.kHardware

    if (isHwAccount) {
      await processSignSolTransactionsRequestHardware({
        request: props.request,
      }).unwrap()
    } else {
      await processSignSolTransactionsRequest({
        approved: true,
        id: props.request.id,
        hwSignatures: [],
        error: null,
      }).unwrap()
    }
  }, [
    processSignSolTransactionsRequest,
    processSignSolTransactionsRequestHardware,
    props,
  ])

  // render
  return {
    cancelSign,
    sign,
  }
}

export const useSignSolanaTransactionsQueue = () => {
  // state
  const [queueNumber, setQueueNumber] = React.useState<number>(1)
  const queueIndex = queueNumber - 1

  // queries
  const { data: signSolTransactionsRequests } =
    useGetPendingSignSolTransactionsRequestsQuery()
  const selectedRequest = signSolTransactionsRequests
    ? signSolTransactionsRequests.at(queueIndex)
    : undefined

  const { account } = useAccountQuery(selectedRequest?.fromAccountId)

  // computed
  const queueLength = signSolTransactionsRequests?.length || 0

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
