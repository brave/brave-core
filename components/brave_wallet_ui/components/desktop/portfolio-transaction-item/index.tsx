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
import { ParsedTransaction } from '../../../common/hooks/transaction-parser'

// Utils
import { toProperCase } from '../../../utils/string-utils'
import { formatDateAsRelative, serializedTimeDeltaToJSDate } from '../../../utils/datetime-utils'
import Amount from '../../../utils/amount'
import { copyToClipboard } from '../../../utils/copy-to-clipboard'
import { getLocale } from '../../../../common/locale'
import { WalletActions } from '../../../common/actions'
import { getTransactionStatusString } from '../../../utils/tx-utils'
import { findTokenBySymbol } from '../../../utils/asset-utils'
import { accountInfoEntityAdaptorInitialState, makeSelectNetworkByIdFromQuery } from '../../../common/slices/entities/account-info.entity'
import { selectAllUserAssetsFromQueryResult } from '../../../common/slices/entities/blockchain-token.entity'

// Hooks
import { useExplorer } from '../../../common/hooks'
import {
  useGetAccountInfosRegistryQuery,
  useGetAllNetworksQuery,
  useGetDefaultFiatCurrencyQuery,
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
  OrbWrapper
} from './style'
import { StatusBubble } from '../../shared/style'
import TransactionFeesTooltip from '../transaction-fees-tooltip'
import TransactionPopup, { TransactionPopupItem } from '../transaction-popup'
import TransactionTimestampTooltip from '../transaction-timestamp-tooltip'

export interface Props {
  transaction: ParsedTransaction
  displayAccountName: boolean
  isFocused?: boolean
}

export const PortfolioTransactionItem = React.forwardRef<HTMLDivElement, Props>(({
  transaction,
  displayAccountName,
  isFocused
}: Props, forwardedRef) => {
  const {
    isFilecoinTransaction,
    isSolanaTransaction,
    fiatValue,
    formattedNativeCurrencyTotal,
    gasFeeFiat
  } = transaction

  // routing
  const history = useHistory()

  // redux
  const dispatch = useDispatch()

  // selectors
  const selectNetworkById = React.useMemo(makeSelectNetworkByIdFromQuery, [])

  // queries
  const { data: defaultFiatCurrency = '' } = useGetDefaultFiatCurrencyQuery(undefined)

  const { txNetwork } = useGetAllNetworksQuery(undefined, {
    selectFromResult: (result) => ({
      txNetwork: selectNetworkById(result, transaction.chainId)
    }),
    skip: !transaction.chainId
  })

  const { userVisibleTokensInfo } = useGetUserTokensRegistryQuery(undefined, {
    selectFromResult: result => ({
      userVisibleTokensInfo: selectAllUserAssetsFromQueryResult(result)
    })
  })

  const {
    data: accountInfosRegistry = accountInfoEntityAdaptorInitialState
  } = useGetAccountInfosRegistryQuery(undefined)
  const account = accountInfosRegistry.entities[transaction.accountAddress]

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

    const account = accountInfosRegistry.entities[address]

    if (account !== undefined) {
      onSelectAccount(account)
      return
    }

    onClickViewOnBlockExplorer('address', address)
  }, [onSelectAccount, accountInfosRegistry.entities, onClickViewOnBlockExplorer])

  const onSelectAsset = React.useCallback((asset: BraveWallet.BlockchainToken) => {
    if (asset.contractAddress === '') {
      history.push(`${WalletRoutes.Portfolio}/${asset.symbol}`)
      return
    }
    history.push(`${WalletRoutes.Portfolio}/${asset.contractAddress}`)
  }, [history])

  const onAssetClick = React.useCallback((symbol?: string) => () => {
    if (!symbol) {
      return
    }

    const asset = findTokenBySymbol(symbol, userVisibleTokensInfo)
    if (asset) {
      onSelectAsset(asset)
    }
  }, [onSelectAsset, userVisibleTokensInfo])

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

    const text = getLocale('braveWalletTransactionSent')
    return (
      <>
        {displayAccountName ? text : toProperCase(text)}{' '}
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
    <PortfolioTransactionItemWrapper ref={forwardedRef} isFocused={isFocused} onClick={onHideTransactionPopup}>
      <OrbAndTxDescriptionContainer>
        <OrbWrapper>
          <FromCircle orb={fromOrb} />
          <ToCircle orb={toOrb} />
        </OrbWrapper>

        <DetailColumn>
          <DetailRow>

            {displayAccountName && // Display account name only if rendered under Portfolio view
              <DetailTextLight>
                {account?.name}
              </DetailTextLight>
            }

            <DetailTextDark>{transactionActionLocale}</DetailTextDark>
            <DetailTextLight>-</DetailTextLight>

            <TransactionTimestampTooltip
              text={
                <TransactionFeeTooltipBody>
                  {serializedTimeDeltaToJSDate(transaction.createdTime).toUTCString()}
                </TransactionFeeTooltipBody>
              }
            >
              <DetailTextDarkBold>
                {formatDateAsRelative(serializedTimeDeltaToJSDate(transaction.createdTime))}
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
        <DetailRow>
          <BalanceColumn>
            <DetailTextDark>
              {/* We need to return a Transaction Time Stamp to calculate Fiat value here */}
              {formattedTransactionFiatValue}
            </DetailTextDark>
            <DetailTextLight>{formattedNativeCurrencyTotal}</DetailTextLight>
          </BalanceColumn>

          {/* Will remove this conditional for solana once https://github.com/brave/brave-browser/issues/22040 is implemented. */}
          {!isSolanaTransaction && !!txNetwork &&
            <TransactionFeesTooltip
              text={
                <>
                  <TransactionFeeTooltipTitle>{getLocale('braveWalletAllowSpendTransactionFee')}</TransactionFeeTooltipTitle>
                  <TransactionFeeTooltipBody>
                    {
                      txNetwork && new Amount(transaction.gasFee)
                        .divideByDecimals(txNetwork.decimals)
                        .formatAsAsset(6, txNetwork.symbol)
                    }
                  </TransactionFeeTooltipBody>
                  <TransactionFeeTooltipBody>
                    {formattedGasFeeFiatValue}
                  </TransactionFeeTooltipBody>
                </>
              }
            >
              <CoinsButton>
                <CoinsIcon />
              </CoinsButton>
            </TransactionFeesTooltip>
          }

          {wasTxRejected
            ? <MoreButton onClick={onShowTransactionPopup}>
              <MoreIcon />
            </MoreButton>
            : <RejectedTransactionSpacer />
          }

          {showTransactionPopup &&
            <TransactionPopup>
              {[
                BraveWallet.TransactionStatus.Approved,
                BraveWallet.TransactionStatus.Submitted,
                BraveWallet.TransactionStatus.Confirmed,
                BraveWallet.TransactionStatus.Dropped
              ].includes(transaction.status) &&
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
              }

              {[
                BraveWallet.TransactionStatus.Submitted,
                BraveWallet.TransactionStatus.Approved
              ].includes(transaction.status) &&
                !isSolanaTransaction &&
                !isFilecoinTransaction &&
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
              }

              {BraveWallet.TransactionStatus.Error === transaction.status &&
                !isSolanaTransaction &&
                !isFilecoinTransaction &&
                <TransactionPopupItem
                  onClick={onClickRetryTransaction}
                  text={getLocale('braveWalletTransactionRetry')}
                />
              }
            </TransactionPopup>
          }
        </DetailRow>
      </StatusBalanceAndMoreContainer>

    </PortfolioTransactionItemWrapper>
  )
}
)

export default PortfolioTransactionItem
