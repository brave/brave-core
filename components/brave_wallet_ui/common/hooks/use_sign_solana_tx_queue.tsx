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
import { getTxDatasFromQueuedSolSignRequest } from '../../utils/tx-utils'

// hooks
import { useUnsafePanelSelector } from './use-safe-selector'
import {
  useGetNetworkQuery
} from '../slices/api.slice'
import { useAccountQuery } from '../slices/api.slice.extra'

export const useProcessSignSolanaTransaction = (props: {
  signMode: 'signTx' | 'signAllTxs'
  account?: BraveWallet.AccountInfo
  request?:
    | BraveWallet.SignTransactionRequest
    | BraveWallet.SignAllTransactionsRequest
}) => {
  // redux
  const dispatch = useDispatch()

  // methods
  const cancelSign = React.useCallback(() => {
    if (!props.request) {
      return
    }

    const payload = { approved: false, id: props.request.id }

    dispatch(
      props.signMode === 'signTx'
        ? PanelActions.signTransactionProcessed(payload)
        : PanelActions.signAllTransactionsProcessed(payload)
    )
  }, [props])

  const sign = React.useCallback(() => {
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
        dispatch(
          PanelActions.signAllTransactionsHardware({
            account: props.account,
            request: props.request as BraveWallet.SignAllTransactionsRequest
          })
        )
        return
      }
      dispatch(
        PanelActions.signAllTransactionsProcessed({
          approved: true,
          id: props.request.id
        })
      )
    }
  }, [props])

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
  const signAllTransactionsRequests = useUnsafePanelSelector(
    PanelSelectors.signAllTransactionsRequests
  )
  const signTransactionQueue =
    signMode === 'signTx'
      ? signTransactionRequests
      : signAllTransactionsRequests

  // state
  const [queueNumber, setQueueNumber] = React.useState<number>(1)

  // computed
  const queueLength = signTransactionQueue.length
  const queueIndex = queueNumber - 1
  const selectedQueueData = signTransactionQueue.at(queueIndex)

  // force signing messages in-order
  const isDisabled = queueNumber !== 1

  // queries
  const { data: network } = useGetNetworkQuery(selectedQueueData ?? skipToken)
  const { account } = useAccountQuery(selectedQueueData?.fromAccountId)

  // memos
  const txDatas = React.useMemo(() => {
    return selectedQueueData
      ? getTxDatasFromQueuedSolSignRequest(selectedQueueData)
      : []
  }, [selectedQueueData])

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
    txDatas,
    signingAccount: account,
    queueNextSignTransaction,
    queueLength,
    queueNumber,
    queueIndex
  }
}
