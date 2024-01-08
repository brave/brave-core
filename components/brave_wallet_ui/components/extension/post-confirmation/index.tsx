// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { useDispatch } from 'react-redux'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Constants
import { BraveWallet } from '../../../constants/types'

// Utils
import { getLocale } from '$web-common/locale'
import {
  findTransactionToken,
  getETHSwapTransactionBuyAndSellTokens,
  getFormattedTransactionTransferredValue,
  getTransactionErc721TokenId,
  getTransactionIntent
} from '../../../utils/tx-utils'
import { makeNetworkAsset } from '../../../options/asset-options'

// Hooks
import { useTransactionsNetwork } from '../../../common/hooks/use-transactions-network'
import { usePendingTransactions } from '../../../common/hooks/use-pending-transaction'
import { useGetTransactionsQuery } from '../../../common/slices/api.slice'
import { useUnsafeUISelector } from '../../../common/hooks/use-safe-selector'
import {
  useAccountQuery,
  useGetCombinedTokensListQuery
} from '../../../common/slices/api.slice.extra'

// Actions
import * as WalletPanelActions from '../../../panel/actions/wallet_panel_actions'

// Components
import { Panel } from '../panel/index'
import { TransactionSubmittedOrSigned } from './submitted_or_signed'
import { TransactionComplete } from './complete'
import { TransactionFailed } from './failed'
import { Loader } from './common/common.style'
import { Skeleton } from '../../shared/loading-skeleton/styles'
import { UISelectors } from '../../../common/selectors'

interface Props {
  transactionId: string
}

export function TransactionStatus(props: Props) {
  const { transactionId } = props

  // redux
  const transactionProviderErrorRegistry = useUnsafeUISelector(
    UISelectors.transactionProviderErrorRegistry
  )

  // queries
  const { tx } = useGetTransactionsQuery(
    transactionId
      ? {
          accountId: null,
          chainId: null,
          coinType: null
        }
      : skipToken,
    {
      selectFromResult: (res) => ({
        isLoading: res.isLoading,
        tx: res.data?.find((tx) => tx.id === transactionId),
        error: res.error as string | undefined
      })
    }
  )
  const { account: txAccount } = useAccountQuery(tx?.fromAccountId)

  const { data: combinedTokensList } = useGetCombinedTokensListQuery()

  // hooks
  const dispatch = useDispatch()
  const transactionNetwork = useTransactionsNetwork(tx)
  const { transactionsQueueLength } = usePendingTransactions()

  // memos
  const networkAsset = React.useMemo(() => {
    return makeNetworkAsset(transactionNetwork)
  }, [transactionNetwork])

  const transactionIntent = React.useMemo(() => {
    if (!tx) {
      return ''
    }

    const token = findTransactionToken(tx, combinedTokensList)

    const { buyAmount, sellAmount, buyToken, sellToken } =
      getETHSwapTransactionBuyAndSellTokens({
        tokensList: combinedTokensList,
        tx,
        nativeAsset: networkAsset
      })

    const { normalizedTransferredValue } =
      getFormattedTransactionTransferredValue({
        tx,
        txAccount,
        txNetwork: transactionNetwork,
        token,
        sellToken
      })

    return getTransactionIntent({
      tx,
      normalizedTransferredValue,
      buyAmount,
      buyToken,
      erc721TokenId: getTransactionErc721TokenId(tx),
      sellAmount,
      sellToken,
      token,
      transactionNetwork
    })
  }, [tx, transactionNetwork, combinedTokensList])

  // methods
  const viewTransactionDetail = React.useCallback(() => {
    if (!tx?.id) {
      return
    }
    dispatch(WalletPanelActions.setSelectedTransactionId(tx.id))
    dispatch(WalletPanelActions.navigateTo('transactionDetails'))
  }, [tx?.id])
  const onClose = () =>
    dispatch(WalletPanelActions.setSelectedTransactionId(undefined))
  const completePrimaryCTAText =
    transactionsQueueLength === 0
      ? getLocale('braveWalletButtonClose')
      : getLocale('braveWalletButtonNext')

  // render
  if (!tx) {
    return <Skeleton />
  }

  if (
    tx.txStatus === BraveWallet.TransactionStatus.Submitted ||
    tx.txStatus === BraveWallet.TransactionStatus.Signed
  ) {
    return (
      <TransactionSubmittedOrSigned
        headerTitle={transactionIntent}
        transaction={tx}
        onClose={onClose}
      />
    )
  }

  if (tx.txStatus === BraveWallet.TransactionStatus.Confirmed) {
    return (
      <TransactionComplete
        headerTitle={transactionIntent}
        description={getLocale('braveWalletTransactionCompleteDescription')}
        isPrimaryCTADisabled={false}
        onClose={onClose}
        onClickSecondaryCTA={viewTransactionDetail}
        onClickPrimaryCTA={onClose}
        primaryCTAText={completePrimaryCTAText}
      />
    )
  }

  if (tx.txStatus === BraveWallet.TransactionStatus.Error) {
    const providerError = transactionProviderErrorRegistry[tx.id]
    const errorDetailContent =
      providerError && `${providerError.code}: ${providerError.message}`
    return (
      <TransactionFailed
        headerTitle={transactionIntent}
        isPrimaryCTADisabled={false}
        errorDetailTitle={getLocale(
          'braveWalletTransactionFailedModalSubtitle'
        )}
        errorDetailContent={errorDetailContent}
        onClose={onClose}
        onClickPrimaryCTA={onClose}
      />
    )
  }

  return (
    <Panel
      navAction={onClose}
      title={transactionIntent}
      headerStyle='slim'
    >
      <Loader />
    </Panel>
  )
}
