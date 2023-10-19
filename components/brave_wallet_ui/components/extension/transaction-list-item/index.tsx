// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Utils
import { getLocale } from '../../../../common/locale'
import { toProperCase } from '../../../utils/string-utils'
import {
  getTransactionGasFee,
  getTransactionStatusString,
  isSolanaTransaction,
  parseTransactionWithPrices,
  findTransactionToken,
  getETHSwapTransactionBuyAndSellTokens
} from '../../../utils/tx-utils'
import { getPriceIdForToken } from '../../../utils/api-utils'
import {
  formatDateAsRelative,
  serializedTimeDeltaToJSDate
} from '../../../utils/datetime-utils'
import { reduceAddress } from '../../../utils/reduce-address'
import Amount from '../../../utils/amount'
import { makeNetworkAsset } from '../../../options/asset-options'

// Types
import {
  BraveWallet,
  SerializableTransactionInfo
} from '../../../constants/types'
import { SwapExchangeProxy } from '../../../common/constants/registry'

// Hooks
import {
  useGetAccountInfosRegistryQuery,
  useGetDefaultFiatCurrencyQuery,
  useGetNetworkQuery,
  useGetSolanaEstimatedFeeQuery,
  useGetTokenSpotPricesQuery
} from '../../../common/slices/api.slice'
import {
  useAccountQuery,
  useGetCombinedTokensListQuery
} from '../../../common/slices/api.slice.extra'
import { querySubscriptionOptions60s } from '../../../common/slices/constants'
import { useAccountOrb, useAddressOrb } from '../../../common/hooks/use-orb'

// Components
import { TransactionIntentDescription } from './transaction-intent-description'

// Styled Components
import { DetailTextDarkBold, DetailTextDark } from '../shared-panel-styles'
import { StatusBubble } from '../../shared/style'
import {
  DetailColumn,
  FromCircle,
  StatusAndTimeRow,
  StatusRow,
  StyledWrapper,
  ToCircle,
  TransactionDetailRow
} from './style'
import { getCoinFromTxDataUnion } from '../../../utils/network-utils'

interface Props {
  transaction: SerializableTransactionInfo
  onSelectTransaction: (transaction: SerializableTransactionInfo) => void
}

const { ERC20Approve, ERC721TransferFrom, ERC721SafeTransferFrom } =
  BraveWallet.TransactionType

export const TransactionsListItem = ({
  transaction,
  onSelectTransaction
}: Props) => {
  const txCoinType = getCoinFromTxDataUnion(transaction.txDataUnion)
  const isSolTx = isSolanaTransaction(transaction)

  // queries & query args
  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()
  const { data: combinedTokensList } = useGetCombinedTokensListQuery()
  const { data: transactionsNetwork } = useGetNetworkQuery({
    chainId: transaction.chainId,
    coin: txCoinType
  })
  const { data: accounts } = useGetAccountInfosRegistryQuery()
  const { account: transactionAccount } = useAccountQuery(
    transaction.fromAccountId
  )

  const txToken = findTransactionToken(transaction, combinedTokensList)

  const networkAsset = React.useMemo(() => {
    return makeNetworkAsset(transactionsNetwork)
  }, [transactionsNetwork])

  const { buyToken, sellToken } = getETHSwapTransactionBuyAndSellTokens({
    tokensList: combinedTokensList,
    tx: transaction,
    nativeAsset: networkAsset
  })

  const tokenPriceIds = React.useMemo(
    () =>
      [txToken, networkAsset, buyToken, sellToken]
        .filter((t): t is BraveWallet.BlockchainToken => Boolean(t))
        .map(getPriceIdForToken),
    [txToken, networkAsset, combinedTokensList]
  )

  const { data: spotPriceRegistry = {} } = useGetTokenSpotPricesQuery(
    tokenPriceIds.length && defaultFiatCurrency
      ? { ids: tokenPriceIds, toCurrency: defaultFiatCurrency }
      : skipToken,
    querySubscriptionOptions60s
  )

  const { data: solEstimatedFee = '' } = useGetSolanaEstimatedFeeQuery(
    isSolTx && transaction.id && transaction.chainId
      ? {
          chainId: transaction.chainId,
          txId: transaction.id
        }
      : skipToken
  )

  // methods
  const onClickTransaction = () => {
    onSelectTransaction(transaction)
  }

  // memos & custom hooks
  const gasFee = isSolTx ? solEstimatedFee : getTransactionGasFee(transaction)

  const transactionDetails = React.useMemo(() => {
    if (
      !spotPriceRegistry ||
      !transactionsNetwork ||
      !transactionAccount ||
      !accounts
    )
      return undefined

    return parseTransactionWithPrices({
      tx: transaction,
      accounts,
      gasFee,
      spotPriceRegistry,
      transactionAccount,
      transactionNetwork: transactionsNetwork,
      tokensList: combinedTokensList
    })
  }, [
    accounts,
    gasFee,
    spotPriceRegistry,
    transaction,
    transactionAccount,
    transactionsNetwork,
    combinedTokensList
  ])

  const fromOrb = useAccountOrb(transactionAccount)
  const toOrb = useAddressOrb(transactionDetails?.recipient)

  const transactionIntentLocale = React.useMemo((): React.ReactNode => {
    if (!transactionDetails) {
      return ''
    }

    // approval
    if (transaction.txType === ERC20Approve) {
      return toProperCase(getLocale('braveWalletApprovalTransactionIntent'))
    }

    // Detect sending to 0x Exchange Proxy
    if (transactionDetails.isSwap) {
      return getLocale('braveWalletSwap')
    }

    // default or when: [ETHSend, ERC20Transfer, ERC721TransferFrom, ERC721SafeTransferFrom].includes(transaction.txType)
    let erc721ID =
      transaction.txType === ERC721TransferFrom ||
      transaction.txType === ERC721SafeTransferFrom
        ? ' ' + transactionDetails.erc721TokenId
        : ''

    return (
      <DetailTextDark>
        {`${toProperCase(getLocale('braveWalletTransactionSent'))} ${
          transactionDetails.value
        } ${transactionDetails.symbol} ${erc721ID} (${
          new Amount(transactionDetails.fiatValue).formatAsFiat(
            defaultFiatCurrency
          ) || '...'
        })`}
      </DetailTextDark>
    )
  }, [transaction.txType, transactionDetails, defaultFiatCurrency])

  const transactionIntentDescription = React.useMemo(() => {
    if (!transactionDetails) {
      return ''
    }

    // default or when: [ETHSend, ERC20Transfer, ERC721TransferFrom, ERC721SafeTransferFrom].includes(transaction.txType)
    // TODO(apaymyshev): need description for bitcoin transaction.
    let from = `${reduceAddress(transaction.fromAddress ?? '')} `
    let to = reduceAddress(transactionDetails.recipient)
    const wrapFromText =
      transaction.txType === ERC20Approve ||
      transaction.txDataUnion.ethTxData1559?.baseData.to.toLowerCase() ===
        SwapExchangeProxy

    if (transaction.txType === ERC20Approve) {
      // Approval
      from = transactionDetails.isApprovalUnlimited
        ? `${getLocale('braveWalletTransactionApproveUnlimited')} ${
            transactionDetails.symbol
          }`
        : new Amount(transactionDetails.value).formatAsAsset(
            undefined,
            transactionDetails.symbol
          )
      to = transactionDetails.approvalTargetLabel || ''
    } else if (
      transaction.txDataUnion.ethTxData1559?.baseData.to.toLowerCase() ===
      SwapExchangeProxy
    ) {
      // Brave Swap
      // FIXME: Add as new TransactionType on the service side.
      from = `${transactionDetails.value} ${transactionDetails.symbol}`
      to = transactionDetails.recipientLabel
    }

    return (
      <TransactionIntentDescription
        from={from}
        to={to}
        wrapFrom={wrapFromText}
      />
    )
  }, [transactionDetails])

  // render
  return (
    <StyledWrapper onClick={onClickTransaction}>
      <DetailColumn>
        <TransactionDetailRow>
          <DetailColumn>
            <FromCircle orb={fromOrb} />
            {toOrb && <ToCircle orb={toOrb} />}
          </DetailColumn>

          <DetailColumn>
            <span>
              <DetailTextDark>{transactionIntentLocale}</DetailTextDark>&nbsp;
              {transactionIntentDescription}
            </span>
            <StatusAndTimeRow>
              <DetailTextDarkBold>
                {formatDateAsRelative(
                  serializedTimeDeltaToJSDate(transaction.createdTime)
                )}
              </DetailTextDarkBold>

              <StatusRow>
                <StatusBubble status={transaction.txStatus} />
                <DetailTextDarkBold>
                  {getTransactionStatusString(transaction.txStatus)}
                </DetailTextDarkBold>
              </StatusRow>
            </StatusAndTimeRow>
          </DetailColumn>
        </TransactionDetailRow>
      </DetailColumn>
    </StyledWrapper>
  )
}

export default TransactionsListItem
