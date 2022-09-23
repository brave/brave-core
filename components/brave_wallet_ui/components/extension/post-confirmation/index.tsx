import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'

// Constants
import { BraveWallet, TransactionProviderError, WalletState } from '../../../constants/types'

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
import { TransactionSubmitted } from './submitted'
import { TransactionComplete } from './complete'
import { TransactionFailed } from './failed'
import { Loader } from './common/common.style'

interface Props {
  transaction: BraveWallet.TransactionInfo
}

export function TransactionStatus (props: Props) {
  const { transaction } = props

  // redux
  const { transactions, transactionProviderErrorRegistry } = useSelector(
    (state: { wallet: WalletState }) => state.wallet
  )

  const liveTransaction: BraveWallet.TransactionInfo = React.useMemo(
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

  if (liveTransaction.txStatus === BraveWallet.TransactionStatus.Submitted) {
    return (
      <TransactionSubmitted
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
