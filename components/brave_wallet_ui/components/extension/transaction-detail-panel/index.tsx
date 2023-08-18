// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Hooks
import { useExplorer } from '../../../common/hooks/explorer'
import {
  useGetDefaultFiatCurrencyQuery,
  useGetNetworkQuery,
  useGetSolanaEstimatedFeeQuery,
  useGetTokenSpotPricesQuery,
  walletApi
} from '../../../common/slices/api.slice'
import {
  useUnsafeUISelector,
  useUnsafeWalletSelector
} from '../../../common/hooks/use-safe-selector'
import {
  useGetCombinedTokensListQuery,
  useTransactionQuery
} from '../../../common/slices/api.slice.extra'
import {
  querySubscriptionOptions60s
} from '../../../common/slices/constants'
import { useAddressOrb } from '../../../common/hooks/use-orb'

// Utils
import { makeNetworkAsset } from '../../../options/asset-options'
import { reduceAddress } from '../../../utils/reduce-address'
import {
  getTransactionGasFee,
  getTransactionStatusString,
  getTransactionToAddress,
  isFilecoinTransaction,
  isSolanaTransaction,
  isSwapTransaction,
  parseTransactionWithPrices,
  findTransactionToken,
  getETHSwapTransactionBuyAndSellTokens
} from '../../../utils/tx-utils'
import { getPriceIdForToken } from '../../../utils/api-utils'
import { toProperCase } from '../../../utils/string-utils'
import Amount from '../../../utils/amount'
import { getCoinFromTxDataUnion } from '../../../utils/network-utils'
import { getLocale } from '../../../../common/locale'
import { UISelectors, WalletSelectors } from '../../../common/selectors'
import { serializedTimeDeltaToJSDate } from '../../../utils/datetime-utils'

// Constants
import {
  BraveWallet
} from '../../../constants/types'

// Styled Components
import {
  StyledWrapper,
  OrbContainer,
  FromCircle,
  ToCircle,
  DetailRow,
  DetailTitle,
  DetailButton,
  StatusRow,
  BalanceColumn,
  TransactionValue,
  PanelDescription,
  SpacerText,
  FromToRow,
  AccountNameText,
  ArrowIcon,
  AlertIcon
} from './style'
import {
  DetailTextDarkBold,
  DetailTextDark
} from '../shared-panel-styles'
import Header from '../../buy-send-swap/select-header'
import { StatusBubble } from '../../shared/style'
import { TransactionStatusTooltip } from '../transaction-status-tooltip'
import { Tooltip } from '../../shared/tooltip/index'
import { Skeleton } from '../../shared/loading-skeleton/styles'

interface Props {
  transactionId: string
  visibleTokens: BraveWallet.BlockchainToken[]
  onBack: () => void
}

export const TransactionDetailPanel = (props: Props) => {
  // props
  const {
    transactionId,
    onBack,
  } = props

  // redux
  const dispatch = useDispatch()
  const accounts = useUnsafeWalletSelector(WalletSelectors.accounts)
  const transactionProviderErrorRegistry = useUnsafeUISelector(
    UISelectors.transactionProviderErrorRegistry
  )

  // queries & query args
  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()
  const { data: combinedTokensList } = useGetCombinedTokensListQuery()
  const { transaction } = useTransactionQuery(transactionId || skipToken)
  const txCoinType = transaction
    ? getCoinFromTxDataUnion(transaction.txDataUnion)
    : undefined

  const isSolanaTxn = transaction
    ? isSolanaTransaction(transaction)
    : undefined

  const { data: transactionsNetwork } = useGetNetworkQuery(
    transaction && txCoinType
      ? {
          chainId: transaction.chainId,
          coin: txCoinType
        }
      : skipToken
  )

  const { data: solFeeEstimate } = useGetSolanaEstimatedFeeQuery(
    isSolanaTxn && transaction?.chainId && transaction?.id
      ? {
          chainId: transaction.chainId,
          txId: transaction.id
        }
      : skipToken
  )

  const networkAsset = React.useMemo(() => {
    return makeNetworkAsset(transactionsNetwork)
  }, [transactionsNetwork])

  const txToken = findTransactionToken(transaction, combinedTokensList)
  const { buyToken, sellToken } = getETHSwapTransactionBuyAndSellTokens({
    tx: transaction,
    tokensList: combinedTokensList,
    nativeAsset: networkAsset
  })

  const tokenPriceIds = React.useMemo(
    () =>
      [txToken, networkAsset, buyToken, sellToken]
        .filter((t): t is BraveWallet.BlockchainToken => Boolean(t))
        .map(getPriceIdForToken),
    [txToken, networkAsset, buyToken, sellToken]
  )

  const {
    data: spotPriceRegistry = {}
  } = useGetTokenSpotPricesQuery(
    tokenPriceIds.length && defaultFiatCurrency
      ? { ids: tokenPriceIds, toCurrency: defaultFiatCurrency }
      : skipToken,
    querySubscriptionOptions60s
  )

  // memos
  const gasFee = React.useMemo(() => {
    if (!transaction) {
      return ''
    }

    return txCoinType === BraveWallet.CoinType.SOL
      ? solFeeEstimate ?? ''
      : getTransactionGasFee(transaction)
  }, [transaction, txCoinType, solFeeEstimate])

  const transactionDetails = React.useMemo(() => {
    if (!transaction || !spotPriceRegistry) {
      return undefined
    }

    return parseTransactionWithPrices({
      tx: transaction,
      accounts,
      gasFee,
      spotPriceRegistry,
      tokensList: combinedTokensList,
      transactionNetwork: transactionsNetwork
    })
  }, [
    transaction,
    transactionsNetwork,
    accounts,
    spotPriceRegistry,
    gasFee
  ])

  const { txType } = transaction || {}
  const {
    erc721BlockchainToken,
    fiatValue,
    gasFeeFiat,
    isApprovalUnlimited,
    value: normalizedTransferredValue,
    recipient,
    recipientLabel,
    senderLabel,
    symbol,
  } = transactionDetails || {}

  const fromOrb = useAddressOrb(transaction?.fromAddress)

  const to = transaction ? getTransactionToAddress(transaction) : ''

  const toOrb = useAddressOrb(to)

  const transactionTitle = React.useMemo((): string => {
    if (!transaction) {
      return ''
    }
    if (isSwapTransaction(transaction)) {
      return toProperCase(getLocale('braveWalletSwap'))
    }
    if (transaction?.txType === BraveWallet.TransactionType.ERC20Approve) {
      return toProperCase(getLocale('braveWalletApprovalTransactionIntent'))
    }
    return toProperCase(getLocale('braveWalletTransactionSent'))
  }, [transaction])

  const transactionValue = React.useMemo((): string => {
    if (txType !== undefined || !normalizedTransferredValue) {
      return ''
    }

    if (
      txType === BraveWallet.TransactionType.ERC721TransferFrom ||
      txType === BraveWallet.TransactionType.ERC721SafeTransferFrom
    ) {
      return erc721BlockchainToken?.name + ' ' + erc721BlockchainToken?.tokenId
    }

    if (
      txType === BraveWallet.TransactionType.ERC20Approve &&
      isApprovalUnlimited
    ) {
      return `${getLocale('braveWalletTransactionApproveUnlimited')} ${symbol}`
    }

    return new Amount(normalizedTransferredValue).formatAsAsset(
      undefined,
      symbol
    )
  }, [
    erc721BlockchainToken,
    isApprovalUnlimited,
    symbol,
    txType,
    normalizedTransferredValue
  ])

  const transactionFiatValue = React.useMemo((): string => {
    if (!txType !== undefined || !fiatValue) {
      return ''
    }

    if (
      txType !== BraveWallet.TransactionType.ERC721TransferFrom &&
      txType !== BraveWallet.TransactionType.ERC721SafeTransferFrom &&
      txType !== BraveWallet.TransactionType.ERC20Approve
    ) {
      return new Amount(fiatValue).formatAsFiat(defaultFiatCurrency)
    }
    return ''
  }, [fiatValue, txType, defaultFiatCurrency])

  const isFilTransaction = transaction
    ? isFilecoinTransaction(transaction)
    : undefined
  const txError = transactionProviderErrorRegistry[transactionId]

  // methods
  const onClickViewOnBlockExplorer = useExplorer(transactionsNetwork)

  const onClickRetryTransaction = () => {
    if (transaction && txCoinType) {
      dispatch(
        walletApi.endpoints.retryTransaction.initiate({
          chainId: transaction.chainId,
          coinType: txCoinType,
          fromAddress: transaction.fromAddress,
          transactionId: transaction.id
        })
      )
    }
  }

  const onClickSpeedupTransaction = () => {
    if (transaction && txCoinType) {
      dispatch(
        walletApi.endpoints.speedupTransaction.initiate({
          chainId: transaction.chainId,
          coinType: txCoinType,
          fromAddress: transaction.fromAddress,
          transactionId: transaction.id
        })
      )
    }
  }

  const onClickCancelTransaction = () => {
    if (transaction && txCoinType) {
      dispatch(
        walletApi.endpoints.cancelTransaction.initiate({
          chainId: transaction.chainId,
          coinType: txCoinType,
          fromAddress: transaction.fromAddress,
          transactionId: transaction.id
        })
      )
    }
  }

  // render
  if (!transaction) {
    return <Skeleton />
  }

  return (
    <StyledWrapper>
      <Header
        title={getLocale('braveWalletTransactionDetails')}
        onBack={onBack}
      />
      <OrbContainer>
        <FromCircle orb={fromOrb} />
        <ToCircle orb={toOrb} />
      </OrbContainer>
      <FromToRow>
        <Tooltip
          text={transaction.fromAddress}
          isAddress={true}
          position={'left'}
        >
          <AccountNameText>{senderLabel}</AccountNameText>
        </Tooltip>
        <ArrowIcon />
        <Tooltip text={recipient} isAddress={true} position={'right'}>
          <AccountNameText>{recipientLabel}</AccountNameText>
        </Tooltip>
      </FromToRow>
      <PanelDescription>{transactionTitle}</PanelDescription>
      <TransactionValue>{transactionValue}</TransactionValue>
      <PanelDescription>{transactionFiatValue}</PanelDescription>
      <DetailRow>
        <DetailTitle>
          {getLocale('braveWalletTransactionDetailStatus')}
        </DetailTitle>
        <StatusRow>
          <StatusBubble status={transaction?.txStatus} />
          <DetailTextDarkBold>
            {getTransactionStatusString(transaction?.txStatus)}
          </DetailTextDarkBold>

          {transaction?.txStatus === BraveWallet.TransactionStatus.Error &&
            txError && (
              <TransactionStatusTooltip
                text={`${txError.code}: ${txError.message}`}
              >
                <AlertIcon />
              </TransactionStatusTooltip>
            )}
        </StatusRow>
      </DetailRow>
      {/* Will remove this conditional for solana once https://github.com/brave/brave-browser/issues/22040 is implemented. */}
      {!isSolanaTxn && (
        <DetailRow>
          <DetailTitle>
            {getLocale('braveWalletAllowSpendTransactionFee')}
          </DetailTitle>
          <BalanceColumn>
            <DetailTextDark>
              {gasFee && transactionsNetwork ? (
                new Amount(gasFee)
                  .divideByDecimals(transactionsNetwork.decimals)
                  .formatAsAsset(6, transactionsNetwork.symbol)
              ) : (
                <Skeleton />
              )}
            </DetailTextDark>
            <DetailTextDark>
              {gasFeeFiat ? (
                new Amount(gasFeeFiat).formatAsFiat(defaultFiatCurrency)
              ) : (
                <Skeleton />
              )}
            </DetailTextDark>
          </BalanceColumn>
        </DetailRow>
      )}
      <DetailRow>
        <DetailTitle>
          {getLocale('braveWalletTransactionDetailDate')}
        </DetailTitle>
        <DetailTextDark>
          {serializedTimeDeltaToJSDate(transaction.createdTime).toUTCString()}
        </DetailTextDark>
      </DetailRow>
      {![
        BraveWallet.TransactionStatus.Rejected,
        BraveWallet.TransactionStatus.Error
      ].includes(transaction?.txStatus) && (
        <DetailRow>
          <DetailTitle>
            {getLocale('braveWalletTransactionDetailHash')}
          </DetailTitle>
          <DetailButton
            onClick={onClickViewOnBlockExplorer('tx', transaction?.txHash)}
          >
            {reduceAddress(transaction?.txHash)}
          </DetailButton>
        </DetailRow>
      )}
      <DetailRow>
        <DetailTitle>
          {getLocale('braveWalletTransactionDetailNetwork')}
        </DetailTitle>
        <DetailTextDark>{transactionsNetwork?.chainName ?? ''}</DetailTextDark>
      </DetailRow>

      {[
        BraveWallet.TransactionStatus.Approved,
        BraveWallet.TransactionStatus.Submitted
      ].includes(transaction?.txStatus) &&
        !isSolanaTxn &&
        !isFilTransaction && (
          <DetailRow>
            <DetailTitle />
            <StatusRow>
              <DetailButton onClick={onClickSpeedupTransaction}>
                {getLocale('braveWalletTransactionDetailSpeedUp')}
              </DetailButton>
              <SpacerText>|</SpacerText>
              <DetailButton onClick={onClickCancelTransaction}>
                {getLocale('braveWalletButtonCancel')}
              </DetailButton>
            </StatusRow>
          </DetailRow>
        )}
      {transaction?.txStatus === BraveWallet.TransactionStatus.Error &&
        !isSolanaTxn &&
        !isFilTransaction && (
          <DetailRow>
            <DetailTitle />
            <StatusRow>
              <DetailButton onClick={onClickRetryTransaction}>
                {getLocale('braveWalletTransactionRetry')}
              </DetailButton>
            </StatusRow>
          </DetailRow>
        )}
    </StyledWrapper>
  )
}

export default TransactionDetailPanel
