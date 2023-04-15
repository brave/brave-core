// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import { useHistory } from 'react-router'
import * as EthereumBlockies from 'ethereum-blockies'

// Types
import {
  BraveWallet,
  WalletRoutes
} from '../../../constants/types'

// Utils
import { toProperCase } from '../../../utils/string-utils'
import { formatDateAsRelative, serializedTimeDeltaToJSDate } from '../../../utils/datetime-utils'
import Amount from '../../../utils/amount'
import { copyToClipboard } from '../../../utils/copy-to-clipboard'
import { getLocale } from '../../../../common/locale'
import { WalletActions } from '../../../common/actions'
import {
  getGasFeeFiatValue,
  getTransactionStatusString,
  ParsedTransactionWithoutFiatValues
} from '../../../utils/tx-utils'
import { findTokenBySymbol } from '../../../utils/asset-utils'
import {
  accountInfoEntityAdaptor,
  accountInfoEntityAdaptorInitialState
} from '../../../common/slices/entities/account-info.entity'
import { selectAllUserAssetsFromQueryResult } from '../../../common/slices/entities/blockchain-token.entity'
import { makeNetworkAsset } from '../../../options/asset-options'

// Hooks
import { useExplorer } from '../../../common/hooks'
import {
  useGetAccountInfosRegistryQuery,
  useGetDefaultFiatCurrencyQuery,
  useGetNetworkQuery,
  useGetTokenSpotPriceQuery,
  useGetUserTokensRegistryQuery
} from '../../../common/slices/api.slice'

// Styled Components
import {
  AddressOrAsset,
  ArrowIcon,
  BalanceColumn,
  CoinsButton,
  CoinsIcon,
  DetailColumn,
  DetailRow,
  DetailTextDark,
  DetailTextDarkBold,
  DetailTextLight,
  RejectedTransactionSpacer,
  FromCircle,
  MoreButton,
  MoreIcon,
  StatusRow,
  PortfolioTransactionItemWrapper,
  ToCircle,
  OrbAndTxDescriptionContainer,
  TransactionFeeTooltipBody,
  TransactionFeeTooltipTitle,
  StatusBalanceAndMoreContainer,
  OrbWrapper,
  BalanceAndMoreRow,
  CoinsButtonSpacer
} from './style'
import { StatusBubble } from '../../shared/style'
import TransactionFeesTooltip from '../transaction-fees-tooltip'
import TransactionPopup, { TransactionPopupItem } from '../transaction-popup'
import TransactionTimestampTooltip from '../transaction-timestamp-tooltip'
import { Skeleton, LoadingSkeletonStyleProps } from '../../shared/loading-skeleton/styles'

export interface Props {
  transaction: ParsedTransactionWithoutFiatValues
  displayAccountName: boolean
  isFocused?: boolean
}

const skeletonProps: LoadingSkeletonStyleProps = {
  width: '38px',
  height: '14px',
  enableAnimation: true
}

export const PortfolioTransactionItem = React.forwardRef<HTMLDivElement, Props>(({
  transaction,
  displayAccountName,
  isFocused
}: Props, forwardedRef) => {
  const {
    formattedSendCurrencyTotal,
    gasFee,
    isFilecoinTransaction,
    isSolanaTransaction
  } = transaction

  // routing
  const history = useHistory()

  // redux
  const dispatch = useDispatch()

  // queries
  const {
    data: defaultFiatCurrency = '',
    isLoading: isLoadingDefaultFiatCurrency
  } = useGetDefaultFiatCurrencyQuery(undefined)

  const {
    data: txNetwork,
    isLoading: isLoadingTxNetwork //
  } = useGetNetworkQuery({
    chainId: transaction.chainId,
    coin: transaction.coinType
  })
  const networkAsset = React.useMemo(() => {
    return makeNetworkAsset(txNetwork)
  }, [txNetwork])

  const {
    userVisibleTokensInfo,
    isLoading: isLoadingUserVisibleTokens
  } = useGetUserTokensRegistryQuery(undefined, {
    selectFromResult: result => ({
      ...result,
      userVisibleTokensInfo: selectAllUserAssetsFromQueryResult(result)
    })
  })

  const {
    data: accountInfosRegistry = accountInfoEntityAdaptorInitialState,
    isLoading: isLoadingAccountInfos
  } = useGetAccountInfosRegistryQuery(undefined)

  const account =
    accountInfosRegistry.entities[
      accountInfoEntityAdaptor.selectId({ address: transaction.accountAddress })
    ]

  const {
    fiatValue,
    isLoading: isLoadingTokenSpotPrice
  } = useGetTokenSpotPriceQuery({
    chainId: transaction.token?.chainId || '',
    contractAddress: transaction.token?.contractAddress || '',
    isErc721: transaction.token?.isErc721 || false,
    symbol: transaction.token?.symbol || '',
    tokenId: transaction.token?.tokenId || ''
  }, {
    skip: !transaction.token || !transaction.value,
    // TODO: selector
    selectFromResult: (result) => {
      const price = result.data?.price || '0'
      const fiatValue = new Amount(transaction.value)
        .times(price)
        .value?.toString() || '0'

      return {
        ...result,
        fiatValue
      }
    }
  })

  const {
    gasFeeFiat,
    isLoading: isLoadingGasAssetPrice
  } = useGetTokenSpotPriceQuery({
    chainId: networkAsset?.chainId || '',
    contractAddress: networkAsset?.contractAddress || '',
    isErc721: networkAsset?.isErc721 || false,
    symbol: networkAsset?.symbol || '',
    tokenId: networkAsset?.tokenId || ''
  },
  {
    skip: !networkAsset || !gasFee || !txNetwork,
    // TODO: selector
    selectFromResult: (res) => {
      const price = res.data?.price ?? '0'
      const gasFeeFiat = getGasFeeFiatValue({
        gasFee,
        networkSpotPrice: price,
        txNetwork
      })
      return ({
        ...res,
        gasFeeFiat
      })
    }
  })

  // state
  const [showTransactionPopup, setShowTransactionPopup] = React.useState<boolean>(false)

  // custom hooks
  const onClickViewOnBlockExplorer = useExplorer(txNetwork)

  // methods
  const onShowTransactionPopup = React.useCallback(() => {
    setShowTransactionPopup(true)
  }, [])

  const onHideTransactionPopup = React.useCallback(() => {
    if (showTransactionPopup) {
      setShowTransactionPopup(false)
    }
  }, [showTransactionPopup])

  const onClickCopyTransactionHash = React.useCallback(
    () => copyToClipboard(transaction.hash),
    [transaction.hash]
  )

  const onClickRetryTransaction = React.useCallback(
    () => {
      dispatch(WalletActions.retryTransaction({
        coinType: transaction.coinType,
        fromAddress: transaction.accountAddress,
        transactionId: transaction.id
      }))
    },
    [
      transaction.coinType,
      transaction.accountAddress,
      transaction.id
    ]
  )

  const onClickSpeedupTransaction = React.useCallback(
    () => {
      dispatch(WalletActions.speedupTransaction({
        coinType: transaction.coinType,
        fromAddress: transaction.accountAddress,
        transactionId: transaction.id
      }))
    },
    [
      transaction.coinType,
      transaction.accountAddress,
      transaction.id
    ]
  )

  const onClickCancelTransaction = React.useCallback(
    () => {
      dispatch(WalletActions.cancelTransaction({
        coinType: transaction.coinType,
        fromAddress: transaction.accountAddress,
        transactionId: transaction.id
      }))
    },
    [
      transaction.coinType,
      transaction.accountAddress,
      transaction.id
    ]
  )

  const onSelectAccount = React.useCallback((account: { address: string }) => {
    history.push(`${WalletRoutes.Accounts}/${account.address}`)
  }, [history])

  const onAddressClick = React.useCallback((address?: string) => () => {
    if (!address) {
      return
    }

    const account = accountInfosRegistry.entities[
      accountInfoEntityAdaptor.selectId({ address })
    ]

    if (account !== undefined) {
      onSelectAccount(account)
      return
    }

    onClickViewOnBlockExplorer('address', address)
  }, [onSelectAccount, accountInfosRegistry.entities, onClickViewOnBlockExplorer])

  const onSelectAsset = React.useCallback((asset: BraveWallet.BlockchainToken) => {
    if (asset.contractAddress === '') {
      history.push(`${WalletRoutes.Portfolio}/${asset.chainId}/${asset.symbol}`)
      return
    }
    history.push(`${WalletRoutes.Portfolio}/${asset.chainId}/${asset.contractAddress}/${asset.tokenId}`)
  }, [history])

  const onAssetClick = React.useCallback((symbol?: string) =>
    isLoadingUserVisibleTokens ? undefined : () => {
      if (!symbol) {
        return
      }

      const asset = findTokenBySymbol(symbol, userVisibleTokensInfo)
      if (asset) {
        onSelectAsset(asset)
      }
    },
    [
      onSelectAsset,
      userVisibleTokensInfo,
      isLoadingUserVisibleTokens
    ]
  )

  // memos
  const fromOrb = React.useMemo(() => {
    return EthereumBlockies.create({ seed: transaction.sender.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [transaction.sender])

  const toOrb = React.useMemo(() => {
    return EthereumBlockies.create({ seed: transaction.recipient.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [transaction.recipient])

  const transactionIntentDescription = React.useMemo(() => {
    switch (true) {
      case transaction.txType === BraveWallet.TransactionType.ERC20Approve: {
        const text = getLocale('braveWalletApprovalTransactionIntent')
        return (
          <DetailRow>
            <DetailTextDark>
              {toProperCase(text)} {
                transaction.isApprovalUnlimited
                  ? getLocale('braveWalletTransactionApproveUnlimited')
                  : transaction.value
              }{' '}
              <AddressOrAsset onClick={onAssetClick(transaction.symbol)}>
                {transaction.symbol}
              </AddressOrAsset> -{' '}
              <AddressOrAsset onClick={onAddressClick(transaction.approvalTarget)}>
                {transaction.approvalTargetLabel}
              </AddressOrAsset>
            </DetailTextDark>
          </DetailRow>
        )
      }

      case transaction.txType === BraveWallet.TransactionType.ETHSwap: {
        return (
          <DetailRow>
            <DetailTextDark>
              {transaction.sellAmount?.format(6)}{' '}
              <AddressOrAsset
                onClick={onAssetClick(transaction.sellToken?.symbol)}
              >
                {transaction.sellToken?.symbol}
              </AddressOrAsset>
            </DetailTextDark>
            <ArrowIcon />
            <DetailTextDark>
              {transaction.minBuyAmount?.format(6)}{' '}
              <AddressOrAsset onClick={onAddressClick(transaction.buyToken?.symbol)}>
                {transaction.buyToken?.symbol}
              </AddressOrAsset>
            </DetailTextDark>
          </DetailRow>
        )
      }

      case transaction.txType !== BraveWallet.TransactionType.ETHSwap && transaction.isSwap: {
        return (
          <DetailRow>
            <DetailTextDark>
              {transaction.value}{' '}
              <AddressOrAsset onClick={onAssetClick(transaction.symbol)}>
                {transaction.symbol}
              </AddressOrAsset>
            </DetailTextDark>
            <ArrowIcon />
            <DetailTextDark>
              <AddressOrAsset onClick={onAddressClick(transaction.recipient)}>
                {transaction.recipientLabel}
              </AddressOrAsset>
            </DetailTextDark>
          </DetailRow>
        )
      }

      case transaction.txType === BraveWallet.TransactionType.ETHSend:
      case transaction.txType === BraveWallet.TransactionType.ERC20Transfer:
      case transaction.txType === BraveWallet.TransactionType.ERC721TransferFrom:
      case transaction.txType === BraveWallet.TransactionType.ERC721SafeTransferFrom:
      default: {
        return (
          <DetailRow>
            <DetailTextDark>
              <AddressOrAsset onClick={onAddressClick(transaction.sender)}>
                {transaction.senderLabel}
              </AddressOrAsset>
            </DetailTextDark>
            <ArrowIcon />
            <DetailTextDark>
              <AddressOrAsset onClick={onAddressClick(transaction.recipient)}>
                {transaction.recipientLabel}
              </AddressOrAsset>
            </DetailTextDark>
          </DetailRow>
        )
      }
    }
  }, [transaction, onAssetClick, onAddressClick])

  const transactionActionLocale = React.useMemo(() => {
    if (transaction.isSwap) {
      const text = getLocale('braveWalletSwap')
      return displayAccountName ? text.toLowerCase() : text
    }

    if (transaction.txType === BraveWallet.TransactionType.ERC20Approve) {
      const text = getLocale('braveWalletApprovalTransactionIntent')
      return (
        <>
          {displayAccountName ? text : toProperCase(text)}{' '}
          <AddressOrAsset onClick={onAssetClick(transaction.symbol)}>
            {transaction.symbol}
          </AddressOrAsset>
        </>
      )
    }

    return (
      <>
        {toProperCase(getLocale('braveWalletTransactionSent'))}{' '}
        <AddressOrAsset
          // Disabled for ERC721 tokens until we have NFT meta data
          disabled={
            transaction.txType === BraveWallet.TransactionType.ERC721TransferFrom ||
            transaction.txType === BraveWallet.TransactionType.ERC721SafeTransferFrom
          }
          onClick={onAssetClick(transaction.symbol)}
        >
          {transaction.symbol}
          {
            transaction.txType === BraveWallet.TransactionType.ERC721TransferFrom ||
              transaction.txType === BraveWallet.TransactionType.ERC721SafeTransferFrom
              ? ' ' + transaction.erc721TokenId
              : ''
          }
        </AddressOrAsset>
      </>
    )
  }, [transaction, displayAccountName, onAssetClick])

  const wasTxRejected =
    transaction.status !== BraveWallet.TransactionStatus.Rejected &&
    transaction.status !== BraveWallet.TransactionStatus.Unapproved

  const formattedGasFeeFiatValue = React.useMemo(() => {
    return new Amount(gasFeeFiat).formatAsFiat(defaultFiatCurrency)
  }, [gasFeeFiat, defaultFiatCurrency])

  const formattedTransactionFiatValue = React.useMemo(() => {
    return new Amount(fiatValue).formatAsFiat(defaultFiatCurrency)
  }, [fiatValue, defaultFiatCurrency])

  // render
  return (
    <PortfolioTransactionItemWrapper
      ref={forwardedRef}
      isFocused={isFocused}
      onClick={onHideTransactionPopup}
    >
      <OrbAndTxDescriptionContainer>
        <OrbWrapper>
          <FromCircle orb={fromOrb} />
          <ToCircle orb={toOrb} />
        </OrbWrapper>

        <DetailColumn>
          <DetailRow>
            {/* Display account name only if rendered under Portfolio view */}
            {displayAccountName && account?.name && (
              <DetailTextLight>
                {isLoadingAccountInfos ? (
                  <Skeleton {...skeletonProps} />
                ) : (
                  account.name
                )}
              </DetailTextLight>
            )}

            <DetailTextDark>{transactionActionLocale}</DetailTextDark>
            <DetailTextLight>-</DetailTextLight>

            <TransactionTimestampTooltip
              text={
                <TransactionFeeTooltipBody>
                  {serializedTimeDeltaToJSDate(
                    transaction.createdTime
                  ).toUTCString()}
                </TransactionFeeTooltipBody>
              }
            >
              <DetailTextDarkBold>
                {formatDateAsRelative(
                  serializedTimeDeltaToJSDate(transaction.createdTime)
                )}
              </DetailTextDarkBold>
            </TransactionTimestampTooltip>
          </DetailRow>

          {transactionIntentDescription}
        </DetailColumn>
      </OrbAndTxDescriptionContainer>

      <StatusBalanceAndMoreContainer>
        <StatusRow>
          <StatusBubble status={transaction.status} />
          <DetailTextDarkBold>
            {getTransactionStatusString(transaction.status)}
          </DetailTextDarkBold>
        </StatusRow>

        {/* Balance & more */}
        <BalanceAndMoreRow>
          <BalanceColumn>
            {transaction.txType !==
              BraveWallet.TransactionType.ERC20Approve && (
              <>
                <DetailTextDark>
                  {/*
                    We need to return a Transaction Time Stamp
                    to calculate Fiat value here
                  */}
                  {isLoadingTokenSpotPrice || isLoadingDefaultFiatCurrency ? (
                    <Skeleton {...skeletonProps} />
                  ) : (
                    formattedTransactionFiatValue || 'NOT FOUND!'
                  )}
                </DetailTextDark>
                <DetailTextLight>{formattedSendCurrencyTotal}</DetailTextLight>
              </>
            )}
          </BalanceColumn>

          {/* Will remove this conditional for solana once https://github.com/brave/brave-browser/issues/22040 is implemented. */}
          {(!isSolanaTransaction && !!txNetwork) ? (
            <TransactionFeesTooltip
              text={
                <>
                  <TransactionFeeTooltipTitle>
                    {getLocale('braveWalletAllowSpendTransactionFee')}
                  </TransactionFeeTooltipTitle>
                  <TransactionFeeTooltipBody>
                    {isLoadingTxNetwork ? (
                      <Skeleton {...skeletonProps} />
                    ) : (
                      txNetwork &&
                      new Amount(transaction.gasFee)
                        .divideByDecimals(txNetwork.decimals)
                        .formatAsAsset(6, txNetwork.symbol)
                    )}
                  </TransactionFeeTooltipBody>
                  <TransactionFeeTooltipBody>
                    {isLoadingDefaultFiatCurrency || isLoadingGasAssetPrice ? (
                      <Skeleton {...skeletonProps} />
                    ) : (
                      formattedGasFeeFiatValue
                    )}
                  </TransactionFeeTooltipBody>
                </>
              }
            >
              <CoinsButton>
                <CoinsIcon />
              </CoinsButton>
            </TransactionFeesTooltip>
          ):
            <CoinsButtonSpacer />
          }

          {wasTxRejected ? (
            <MoreButton onClick={onShowTransactionPopup}>
              <MoreIcon />
            </MoreButton>
          ) : (
            <RejectedTransactionSpacer />
          )}

          {showTransactionPopup && (
            <TransactionPopup>
              {[
                BraveWallet.TransactionStatus.Approved,
                BraveWallet.TransactionStatus.Submitted,
                BraveWallet.TransactionStatus.Confirmed,
                BraveWallet.TransactionStatus.Dropped
              ].includes(transaction.status) && (
                <>
                  <TransactionPopupItem
                    onClick={onClickViewOnBlockExplorer('tx', transaction.hash)}
                    text={getLocale('braveWalletTransactionExplorer')}
                  />
                  <TransactionPopupItem
                    onClick={onClickCopyTransactionHash}
                    text={getLocale('braveWalletTransactionCopyHash')}
                  />
                </>
              )}

              {[
                BraveWallet.TransactionStatus.Submitted,
                BraveWallet.TransactionStatus.Approved
              ].includes(transaction.status) &&
                !isSolanaTransaction &&
                !isFilecoinTransaction && (
                  <>
                    <TransactionPopupItem
                      onClick={onClickSpeedupTransaction}
                      text={getLocale('braveWalletTransactionSpeedup')}
                    />
                    <TransactionPopupItem
                      onClick={onClickCancelTransaction}
                      text={getLocale('braveWalletTransactionCancel')}
                    />
                  </>
                )}

              {BraveWallet.TransactionStatus.Error === transaction.status &&
                !isSolanaTransaction &&
                !isFilecoinTransaction && (
                  <TransactionPopupItem
                    onClick={onClickRetryTransaction}
                    text={getLocale('braveWalletTransactionRetry')}
                  />
                )}
            </TransactionPopup>
          )}
        </BalanceAndMoreRow>
      </StatusBalanceAndMoreContainer>
    </PortfolioTransactionItemWrapper>
  )
}
)

export default PortfolioTransactionItem
