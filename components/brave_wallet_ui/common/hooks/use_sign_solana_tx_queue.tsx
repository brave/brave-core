// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import { skipToken } from '@reduxjs/toolkit/query/react'

// types
import { PanelActions } from '../../panel/actions'
import { BraveWallet } from '../../constants/types'

// utils
import { PanelSelectors } from '../../panel/selectors'

// hooks
import { useUnsafePanelSelector } from './use-safe-selector'
import {
  useGetNetworkQuery,
  useGetPendingSignAllTransactionsRequestsQuery,
  useProcessSignAllTransactionsRequestHardwareMutation,
  useProcessSignAllTransactionsRequestMutation
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
  // redux
  const dispatch = useDispatch()

  // mutations
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
      dispatch(PanelActions.signTransactionProcessed(payload))
    } else {
      await signAllTransactions(payload).unwrap()
    }
  }, [dispatch, props.request, props.signMode, signAllTransactions])

  const sign = React.useCallback(async () => {
    if (!props.request || !props.account) {
      return
    }

    const isHwAccount =
      props.account.accountId.kind === BraveWallet.AccountKind.kHardware

    if (props.signMode === 'signTx') {
      if (isHwAccount) {
        dispatch(
          PanelActions.signTransactionHardware({
            account: props.account,
            request: props.request as BraveWallet.SignTransactionRequest
          })
        )
        return
      }
      dispatch(
        PanelActions.signTransactionProcessed({
          approved: true,
          id: props.request.id
        })
      )
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
    dispatch,
    props.account,
    props.request,
    props.signMode,
    signAllTransactions,
    signAllTransactionsHardware
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
  // redux
  const signTransactionRequests = useUnsafePanelSelector(
    PanelSelectors.signTransactionRequests
  )

  // queries
  const { data: signAllTransactionsRequests } =
    useGetPendingSignAllTransactionsRequestsQuery()

  const signTransactionQueue =
    signMode === 'signTx'
      ? signTransactionRequests
      : signAllTransactionsRequests

  // state
  const [queueNumber, setQueueNumber] = React.useState<number>(1)

  // computed
  const queueLength = signTransactionQueue?.length || 0
  const queueIndex = queueNumber - 1
  const selectedQueueData = signTransactionQueue
    ? signTransactionQueue.at(queueIndex)
    : undefined

  // force signing messages in-order
  const isDisabled = queueNumber !== 1

  // queries
  const { data: network } = useGetNetworkQuery(selectedQueueData ?? skipToken)
  const { account } = useAccountQuery(selectedQueueData?.fromAccountId)

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
