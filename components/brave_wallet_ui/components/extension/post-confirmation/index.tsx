// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { useDispatch } from 'react-redux'
import { skipToken } from '@reduxjs/toolkit/query/react'
import { useHistory } from 'react-router'

// Constants
import { BraveWallet, TransactionInfoLookup } from '../../../constants/types'

// Utils
import { getCoinFromTxDataUnion } from '../../../utils/network-utils'
import { makeTransactionDetailsRoute } from '../../../utils/routes-utils'

// Hooks
import { useGetTransactionQuery } from '../../../common/slices/api.slice'

// Actions
import * as WalletPanelActions from '../../../panel/actions/wallet_panel_actions'

// Components
import {
  TransactionSubmittedOrSigned //
} from './submitted_or_signed/submitted_or_signed'
import { TransactionComplete } from './complete/complete'
import {
  TransactionFailedOrCanceled //
} from './failed_or_canceled/failed_or_canceled'
import {
  CancelTransaction //
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

  // State
  const [showCancelTransaction, setShowCancelTransaction] =
    React.useState(false)

  // hooks
  const dispatch = useDispatch()

  // methods
  const onClickViewInActivity = React.useCallback(() => {
    if (!tx?.id) {
      return
    }
    dispatch(
      WalletPanelActions.setSelectedTransactionId({
        chainId: tx.chainId,
        coin: getCoinFromTxDataUnion(tx.txDataUnion),
        id: tx.id
      })
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

  if (
    tx.txStatus === BraveWallet.TransactionStatus.Submitted ||
    tx.txStatus === BraveWallet.TransactionStatus.Signed
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

  if (tx.txStatus === BraveWallet.TransactionStatus.Error) {
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
