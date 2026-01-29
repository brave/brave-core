// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { useHistory } from 'react-router'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Redux
import { useAppDispatch } from '../../../common/hooks/use-redux'

// Constants
import { BraveWallet, TransactionInfoLookup } from '../../../constants/types'

// Utils
import { getCoinFromTxDataUnion } from '../../../utils/network-utils'
import { makeTransactionDetailsRoute } from '../../../utils/routes-utils'

// Hooks
import { useGetTransactionQuery } from '../../../common/slices/api.slice'
import {
  useGate3SwapStatus, //
} from '../../../page/screens/swap/hooks/useGate3SwapStatus'

// Actions
import * as WalletPanelActions from '../../../panel/actions/wallet_panel_actions'

// Components
import {
  TransactionSubmittedOrSigned, //
} from './submitted_or_signed/submitted_or_signed'
import { TransactionComplete } from './complete/complete'
import {
  TransactionFailedOrCanceled, //
} from './failed_or_canceled/failed_or_canceled'
import {
  CancelTransaction, //
} from './cancel_transaction/cancel_transaction'

// Styled Components
import { Loader } from './common/common.style'
import { Skeleton } from '../../shared/loading-skeleton/styles'
import { Column } from '../../shared/style'

interface Props {
  transactionLookup: TransactionInfoLookup
}

export function TransactionStatus({ transactionLookup }: Props) {
  // history
  const history = useHistory()

  // queries
  const { data: tx } = useGetTransactionQuery(transactionLookup ?? skipToken)

  // Check if this is a swap or bridge transaction
  const isSwapOrBridge = !!(tx?.swapInfo && tx.swapInfo.routeId !== '')

  // Get swap status for swap/bridge transactions
  const { status: swapStatus } = useGate3SwapStatus(isSwapOrBridge ? tx : null)

  // State
  const [showCancelTransaction, setShowCancelTransaction] =
    React.useState(false)

  // redux
  const dispatch = useAppDispatch()

  // methods
  const onClickViewInActivity = React.useCallback(() => {
    if (!tx?.id) {
      return
    }
    dispatch(
      WalletPanelActions.setSelectedTransactionId({
        chainId: tx.chainId,
        coin: getCoinFromTxDataUnion(tx.txDataUnion),
        id: tx.id,
      }),
    )
    dispatch(WalletPanelActions.navigateToMain())
    history.push(makeTransactionDetailsRoute(tx.id))
  }, [dispatch, history, tx])

  const onClose = () =>
    dispatch(WalletPanelActions.setSelectedTransactionId(undefined))

  // render
  if (!tx) {
    return <Skeleton />
  }

  if (showCancelTransaction) {
    return (
      <CancelTransaction
        onBack={() => setShowCancelTransaction(false)}
        transaction={tx}
      />
    )
  }

  // For swap/bridge transactions, use effective status based on Gate3 swap status
  if (isSwapOrBridge && swapStatus) {
    // Check if the swap failed (on-chain failure or Gate3 failure)
    const effectiveSwapFailed =
      tx.txStatus === BraveWallet.TransactionStatus.Error
      || tx.txStatus === BraveWallet.TransactionStatus.Dropped
      || swapStatus.status === BraveWallet.Gate3SwapStatusCode.kFailed

    // Check if the swap was refunded
    const effectiveSwapRefunded =
      swapStatus.status === BraveWallet.Gate3SwapStatusCode.kRefunded

    // Check if the swap completed successfully
    const effectiveSwapSuccess =
      swapStatus.status === BraveWallet.Gate3SwapStatusCode.kSuccess

    // Route based on effective status
    if (effectiveSwapFailed || effectiveSwapRefunded) {
      return (
        <TransactionFailedOrCanceled
          transaction={tx}
          onClose={onClose}
          swapStatus={swapStatus}
        />
      )
    }

    if (effectiveSwapSuccess) {
      return (
        <TransactionComplete
          transaction={tx}
          onClose={onClose}
          onClickViewInActivity={onClickViewInActivity}
          swapStatus={swapStatus}
        />
      )
    }

    // kPending/kProcessing or waiting for status - keep showing as "in progress"
    return (
      <TransactionSubmittedOrSigned
        transaction={tx}
        onClose={onClose}
        onShowCancelTransaction={() => setShowCancelTransaction(true)}
        onClickViewInActivity={onClickViewInActivity}
        swapStatus={swapStatus}
      />
    )
  }

  if (
    tx.txStatus === BraveWallet.TransactionStatus.Submitted
    || tx.txStatus === BraveWallet.TransactionStatus.Signed
  ) {
    return (
      <TransactionSubmittedOrSigned
        transaction={tx}
        onClose={onClose}
        onShowCancelTransaction={() => setShowCancelTransaction(true)}
        onClickViewInActivity={onClickViewInActivity}
      />
    )
  }

  if (tx.txStatus === BraveWallet.TransactionStatus.Confirmed) {
    return (
      <TransactionComplete
        transaction={tx}
        onClose={onClose}
        onClickViewInActivity={onClickViewInActivity}
      />
    )
  }

  if (
    tx.txStatus === BraveWallet.TransactionStatus.Error
    || tx.txStatus === BraveWallet.TransactionStatus.Dropped
  ) {
    return (
      <TransactionFailedOrCanceled
        transaction={tx}
        onClose={onClose}
      />
    )
  }

  return (
    <Column
      fullHeight={true}
      fullWidth={true}
    >
      <Loader />
    </Column>
  )
}
