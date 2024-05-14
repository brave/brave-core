// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// types
import { BraveWallet } from '../../constants/types'

// hooks
import {
  useGetNetworkQuery,
  useGetPendingSignAllTransactionsRequestsQuery,
  useGetPendingSignTransactionRequestsQuery,
  useProcessSignAllTransactionsRequestHardwareMutation,
  useProcessSignAllTransactionsRequestMutation,
  useProcessSignTransactionRequestHardwareMutation,
  useProcessSignTransactionRequestMutation
} from '../slices/api.slice'
import { useAccountQuery } from '../slices/api.slice.extra'

export interface UseProcessSolTxProps {
  account?: BraveWallet.AccountInfo
  signMode: 'signTx' | 'signAllTxs'
  request?:
    | BraveWallet.SignTransactionRequest
    | BraveWallet.SignAllTransactionsRequest
}

export const useProcessSignSolanaTransaction = (
  props: UseProcessSolTxProps
) => {
  // mutations
  const [signTransaction] = useProcessSignTransactionRequestMutation()
  const [signTransactionHardware] =
    useProcessSignTransactionRequestHardwareMutation()
  const [signAllTransactions] = useProcessSignAllTransactionsRequestMutation()
  const [signAllTransactionsHardware] =
    useProcessSignAllTransactionsRequestHardwareMutation()

  // methods
  const cancelSign = React.useCallback(async () => {
    if (!props.request) {
      return
    }

    const payload = { approved: false, id: props.request.id }

    if (props.signMode === 'signTx') {
      await signTransaction(payload).unwrap()
    } else {
      await signAllTransactions(payload).unwrap()
    }
  }, [props.request, props.signMode, signAllTransactions, signTransaction])

  const sign = React.useCallback(async () => {
    if (!props.request || !props.account) {
      return
    }

    const isHwAccount =
      props.account.accountId.kind === BraveWallet.AccountKind.kHardware

    if (props.signMode === 'signTx') {
      if (isHwAccount) {
        await signTransactionHardware({
          account: props.account,
          request: props.request as BraveWallet.SignTransactionRequest
        }).unwrap()
        return
      }
      await signTransaction({
        approved: true,
        id: props.request.id
      }).unwrap()
      return
    }

    if (props.signMode === 'signAllTxs') {
      if (isHwAccount) {
        await signAllTransactionsHardware({
          account: props.account,
          request: props.request as BraveWallet.SignAllTransactionsRequest
        }).unwrap()
        return
      }
      await signAllTransactions({
        approved: true,
        id: props.request.id
      }).unwrap()
    }
  }, [
    props.account,
    props.request,
    props.signMode,
    signAllTransactions,
    signAllTransactionsHardware,
    signTransaction,
    signTransactionHardware
  ])

  // render
  return {
    cancelSign,
    sign
  }
}

export const useSignSolanaTransactionsQueue = (
  signMode: 'signTx' | 'signAllTxs'
) => {
  // state
  const [queueNumber, setQueueNumber] = React.useState<number>(1)
  const queueIndex = queueNumber - 1

  // queries
  const { data: signTransactionRequests } =
    useGetPendingSignTransactionRequestsQuery()
  const { data: signAllTransactionsRequests } =
    useGetPendingSignAllTransactionsRequestsQuery()
  const signTransactionQueue =
    signMode === 'signTx'
      ? signTransactionRequests
      : signAllTransactionsRequests
  const selectedQueueData = signTransactionQueue
    ? signTransactionQueue.at(queueIndex)
    : undefined
  const { data: network } = useGetNetworkQuery(selectedQueueData ?? skipToken)
  const { account } = useAccountQuery(selectedQueueData?.fromAccountId)

  // computed
  const queueLength = signTransactionQueue?.length || 0

  // force signing messages in-order
  const isDisabled = queueNumber !== 1

  // methods
  const queueNextSignTransaction = React.useCallback(() => {
    setQueueNumber((prev) => (prev === queueLength ? 1 : prev + 1))
  }, [queueLength])

  // render
  return {
    signTransactionQueue,
    selectedQueueData,
    isDisabled,
    network,
    signingAccount: account,
    queueNextSignTransaction,
    queueLength,
    queueNumber,
    queueIndex
  }
}
