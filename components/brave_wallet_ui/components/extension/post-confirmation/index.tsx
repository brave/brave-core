// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'

// Constants
import { BraveWallet, SerializableTransactionInfo, TransactionProviderError, WalletState } from '../../../constants/types'

// Utils
import { getLocale } from '$web-common/locale'

// Hooks
import { useTransactionParser } from '../../../common/hooks'
import { useTransactionsNetwork } from '../../../common/hooks/use-transactions-network'
import { usePendingTransactions } from '../../../common/hooks/use-pending-transaction'

// Actions
import * as WalletPanelActions from '../../../panel/actions/wallet_panel_actions'

// Components
import { Panel } from '../index'
import { TransactionSubmittedOrSigned } from './submitted_or_signed'
import { TransactionComplete } from './complete'
import { TransactionFailed } from './failed'
import { Loader } from './common/common.style'

interface Props {
  transaction: SerializableTransactionInfo
}

export function TransactionStatus (props: Props) {
  const { transaction } = props

  // redux
  const { transactions, transactionProviderErrorRegistry } = useSelector(
    (state: { wallet: WalletState }) => state.wallet
  )

  const liveTransaction: SerializableTransactionInfo = React.useMemo(
    () => transactions[transaction.fromAddress].find(tx => tx.id === transaction.id) || transaction,
    [transactions, transaction]
  )

  // hooks
  const dispatch = useDispatch()
  const transactionsNetwork = useTransactionsNetwork(liveTransaction)
  const transactionParser = useTransactionParser(transactionsNetwork)
  const { transactionsQueueLength } = usePendingTransactions()

  const parsedTransaction = React.useMemo(
    () => transactionParser(liveTransaction),
    [transactionParser, liveTransaction]
  )

  const viewTransactionDetail = () => dispatch(WalletPanelActions.navigateTo('transactionDetails'))
  const onClose = () => dispatch(WalletPanelActions.setSelectedTransaction(undefined))
  const completePrimaryCTAText =
    transactionsQueueLength === 0
      ? getLocale('braveWalletButtonClose')
      : getLocale('braveWalletButtonNext')

  if (liveTransaction.txStatus === BraveWallet.TransactionStatus.Submitted ||
      liveTransaction.txStatus === BraveWallet.TransactionStatus.Signed) {
    return (
      <TransactionSubmittedOrSigned
        headerTitle={parsedTransaction.intent}
        transaction={liveTransaction}
        onClose={onClose}
      />
    )
  }

  if (liveTransaction.txStatus === BraveWallet.TransactionStatus.Confirmed) {
    return (
      <TransactionComplete
        headerTitle={parsedTransaction.intent}
        description={getLocale('braveWalletTransactionCompleteDescription')}
        isPrimaryCTADisabled={false}
        onClose={onClose}
        onClickSecondaryCTA={viewTransactionDetail}
        onClickPrimaryCTA={onClose}
        primaryCTAText={completePrimaryCTAText}
      />
    )
  }

  if (liveTransaction.txStatus === BraveWallet.TransactionStatus.Error) {
    const providerError: TransactionProviderError | undefined =
      transactionProviderErrorRegistry[liveTransaction.id]
    const errorDetailContent = providerError && `${providerError.code}: ${providerError.message}`

    return (
      <TransactionFailed
        headerTitle={parsedTransaction.intent}
        isPrimaryCTADisabled={false}
        errorDetailTitle={getLocale('braveWalletTransactionFailedModalSubtitle')}
        errorDetailContent={errorDetailContent}
        onClose={onClose}
        onClickPrimaryCTA={onClose}
      />
    )
  }

  return (
    <Panel navAction={onClose} title={parsedTransaction.intent} headerStyle='slim'>
      <Loader />
    </Panel>
  )
}
