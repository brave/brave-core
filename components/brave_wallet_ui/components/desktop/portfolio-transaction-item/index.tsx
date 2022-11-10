// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'
import { useHistory } from 'react-router'
import * as EthereumBlockies from 'ethereum-blockies'

import {
  BraveWallet,
  WalletAccountType,
  WalletState,
  WalletRoutes
} from '../../../constants/types'

// Utils
import { toProperCase } from '../../../utils/string-utils'
import { formatDateAsRelative } from '../../../utils/datetime-utils'
import { mojoTimeDeltaToJSDate } from '../../../../common/mojomUtils'
import Amount from '../../../utils/amount'
import { copyToClipboard } from '../../../utils/copy-to-clipboard'
import { getCoinFromTxDataUnion } from '../../../utils/network-utils'
import { getLocale } from '../../../../common/locale'
import { isSolanaTransaction } from '../../../utils/tx-utils'

// Hooks
import { useExplorer, useTransactionParser } from '../../../common/hooks'
import { SwapExchangeProxy } from '../../../common/hooks/address-labels'
import { useTransactionsNetwork } from '../../../common/hooks/use-transactions-network'

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
import { WalletActions } from '../../../common/actions'

export interface Props {
  transaction: BraveWallet.TransactionInfo
  account: WalletAccountType | undefined
  accounts: WalletAccountType[]
  displayAccountName: boolean
  isFocused?: boolean
}

const getLocaleKeyForTxStatus = (status: BraveWallet.TransactionStatus) => {
  switch (status) {
    case BraveWallet.TransactionStatus.Unapproved: return 'braveWalletTransactionStatusUnapproved'
    case BraveWallet.TransactionStatus.Approved: return 'braveWalletTransactionStatusApproved'
    case BraveWallet.TransactionStatus.Rejected: return 'braveWalletTransactionStatusRejected'
    case BraveWallet.TransactionStatus.Submitted: return 'braveWalletTransactionStatusSubmitted'
    case BraveWallet.TransactionStatus.Confirmed: return 'braveWalletTransactionStatusConfirmed'
    case BraveWallet.TransactionStatus.Error: return 'braveWalletTransactionStatusError'
    case BraveWallet.TransactionStatus.Dropped: return 'braveWalletTransactionStatusDropped'
    default: return ''
  }
}

export const PortfolioTransactionItem = React.forwardRef<HTMLDivElement, Props>(({
  transaction,
  account,
  displayAccountName,
  accounts,
  isFocused
}: Props, forwardedRef) => {
  // routing
  const history = useHistory()

  // redux
  const dispatch = useDispatch()
  const {
    defaultCurrencies,
    userVisibleTokensInfo
  } = useSelector(({ wallet }: { wallet: WalletState }) => wallet)

  // state
  const [showTransactionPopup, setShowTransactionPopup] = React.useState<boolean>(false)

  const isSolanaTxn = isSolanaTransaction(transaction)
  const isFilecoinTransaction = getCoinFromTxDataUnion(transaction.txDataUnion) === BraveWallet.CoinType.FIL

  // custom hooks
  const transactionsNetwork = useTransactionsNetwork(transaction)
  const onClickViewOnBlockExplorer = useExplorer(transactionsNetwork)
  const parseTransaction = useTransactionParser(transactionsNetwork)

  // methods
  const onShowTransactionPopup = () => setShowTransactionPopup(true)

  const onClickCopyTransactionHash = React.useCallback(
    () => copyToClipboard(transaction.txHash),
    [transaction.txHash]
  )

  const onClickRetryTransaction = React.useCallback(
    () => dispatch(WalletActions.retryTransaction(transaction)),
    [transaction]
  )

  const onClickSpeedupTransaction = React.useCallback(
    () => dispatch(WalletActions.speedupTransaction(transaction)),
    [transaction]
  )

  const onClickCancelTransaction = React.useCallback(
    () => dispatch(WalletActions.cancelTransaction(transaction)),
    [transaction]
  )

  const onHideTransactionPopup = React.useCallback(() => {
    if (showTransactionPopup) {
      setShowTransactionPopup(false)
    }
  }, [showTransactionPopup])

  const findWalletAccount = React.useCallback((address: string) => {
    return accounts.find((account) => account.address.toLowerCase() === address.toLowerCase())
  }, [accounts])

  const onSelectAccount = React.useCallback((account: WalletAccountType) => {
    history.push(`${WalletRoutes.Accounts}/${account.address}`)
  }, [history])

  const onAddressClick = React.useCallback((address?: string) => () => {
    if (!address) {
      return
    }

    const account = findWalletAccount(address)

    if (account !== undefined) {
      onSelectAccount(account)
      return
    }

    onClickViewOnBlockExplorer('address', address)
  }, [onSelectAccount, findWalletAccount, onClickViewOnBlockExplorer])

  const findToken = React.useCallback((symbol: string) => {
    return userVisibleTokensInfo.find((token) => token.symbol.toLowerCase() === symbol.toLowerCase())
  }, [userVisibleTokensInfo])

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

    const asset = findToken(symbol)
    if (asset) {
      onSelectAsset(asset)
    }
  }, [onSelectAsset, findToken])

  // memos
  const transactionDetails = React.useMemo(
    () => parseTransaction(transaction),
    [transaction, parseTransaction]
  )

  const fromOrb = React.useMemo(() => {
    return EthereumBlockies.create({ seed: transactionDetails.sender.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [transactionDetails.sender])

  const toOrb = React.useMemo(() => {
    return EthereumBlockies.create({ seed: transactionDetails.recipient.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [transactionDetails.recipient])

  const transactionIntentDescription = React.useMemo(() => {
    switch (true) {
      case transaction.txType === BraveWallet.TransactionType.ERC20Approve: {
        const text = getLocale('braveWalletApprovalTransactionIntent')
        return (
          <DetailRow>
            <DetailTextDark>
              {toProperCase(text)} {
                transactionDetails.isApprovalUnlimited
                  ? getLocale('braveWalletTransactionApproveUnlimited')
                  : transactionDetails.value
              }{' '}
              <AddressOrAsset onClick={onAssetClick(transactionDetails.symbol)}>
                {transactionDetails.symbol}
              </AddressOrAsset> -{' '}
              <AddressOrAsset onClick={onAddressClick(transactionDetails.approvalTarget)}>
                {transactionDetails.approvalTargetLabel}
              </AddressOrAsset>
            </DetailTextDark>
          </DetailRow>
        )
      }

      case transaction.txType === BraveWallet.TransactionType.ETHSwap: {
        return (
          <DetailRow>
            <DetailTextDark>
              {transactionDetails.sellAmount?.format(6)}{' '}
              <AddressOrAsset
                onClick={onAssetClick(transactionDetails.sellToken?.symbol)}
              >
                {transactionDetails.sellToken?.symbol}
              </AddressOrAsset>
            </DetailTextDark>
            <ArrowIcon />
            <DetailTextDark>
              {transactionDetails.minBuyAmount?.format(6)}{' '}
              <AddressOrAsset onClick={onAddressClick(transactionDetails.buyToken?.symbol)}>
                {transactionDetails.buyToken?.symbol}
              </AddressOrAsset>
            </DetailTextDark>
          </DetailRow>
        )
      }

      case transaction.txType !== BraveWallet.TransactionType.ETHSwap && transactionDetails.isSwap: {
        return (
          <DetailRow>
            <DetailTextDark>
              {transactionDetails.value}{' '}
              <AddressOrAsset onClick={onAssetClick(transactionDetails.symbol)}>
                {transactionDetails.symbol}
              </AddressOrAsset>
            </DetailTextDark>
            <ArrowIcon />
            <DetailTextDark>
              <AddressOrAsset onClick={onAddressClick(transactionDetails.recipient)}>
                {transactionDetails.recipientLabel}
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
              <AddressOrAsset onClick={onAddressClick(transactionDetails.sender)}>
                {transactionDetails.senderLabel}
              </AddressOrAsset>
            </DetailTextDark>
            <ArrowIcon />
            <DetailTextDark>
              <AddressOrAsset onClick={onAddressClick(transactionDetails.recipient)}>
                {transactionDetails.recipientLabel}
              </AddressOrAsset>
            </DetailTextDark>
          </DetailRow>
        )
      }
    }
  }, [transactionDetails, onAssetClick, onAddressClick])

  const transactionActionLocale = React.useMemo(() => {
    switch (true) {
      case transaction.txType === BraveWallet.TransactionType.ERC20Approve: {
        const text = getLocale('braveWalletApprovalTransactionIntent')
        return (
          <>
            {displayAccountName ? text : toProperCase(text)}{' '}
            <AddressOrAsset onClick={onAssetClick(transactionDetails.symbol)}>
              {transactionDetails.symbol}
            </AddressOrAsset>
          </>
        )
      }

      // Detect sending to 0x Exchange Proxy
      case transaction.txDataUnion.ethTxData1559?.baseData.to.toLowerCase() === SwapExchangeProxy: {
        const text = getLocale('braveWalletSwap')
        return displayAccountName ? text.toLowerCase() : text
      }

      case transaction.txType === BraveWallet.TransactionType.ETHSend:
      case transaction.txType === BraveWallet.TransactionType.ERC20Transfer:
      case transaction.txType === BraveWallet.TransactionType.ERC721TransferFrom:
      case transaction.txType === BraveWallet.TransactionType.ERC721SafeTransferFrom:
      default: {
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
              onClick={onAssetClick(transactionDetails.symbol)}
            >
              {transactionDetails.symbol}
              {
                transaction.txType === BraveWallet.TransactionType.ERC721TransferFrom ||
                  transaction.txType === BraveWallet.TransactionType.ERC721SafeTransferFrom
                  ? ' ' + transactionDetails.erc721TokenId
                  : ''
              }
            </AddressOrAsset>
          </>
        )
      }
    }
  }, [transaction, transactionDetails, displayAccountName, onAssetClick])

  const wasTxRejected =
    transactionDetails.status !== BraveWallet.TransactionStatus.Rejected &&
    transactionDetails.status !== BraveWallet.TransactionStatus.Unapproved

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
                  {mojoTimeDeltaToJSDate(transactionDetails.createdTime).toUTCString()}
                </TransactionFeeTooltipBody>
              }
            >
              <DetailTextDarkBold>
                {formatDateAsRelative(mojoTimeDeltaToJSDate(transactionDetails.createdTime))}
              </DetailTextDarkBold>
            </TransactionTimestampTooltip>

          </DetailRow>

          {transactionIntentDescription}

        </DetailColumn>

      </OrbAndTxDescriptionContainer>

      <StatusBalanceAndMoreContainer>
        <StatusRow>
          <StatusBubble status={transactionDetails.status} />
          <DetailTextDarkBold>
            {getLocale(getLocaleKeyForTxStatus(transactionDetails.status))}
          </DetailTextDarkBold>
        </StatusRow>

        {/* Balance & more */}
        <DetailRow>
          <BalanceColumn>
            <DetailTextDark>
              {/* We need to return a Transaction Time Stamp to calculate Fiat value here */}
              {transactionDetails.fiatValue.formatAsFiat(defaultCurrencies.fiat)}
            </DetailTextDark>
            <DetailTextLight>{transactionDetails.formattedNativeCurrencyTotal}</DetailTextLight>
          </BalanceColumn>

          {/* Will remove this conditional for solana once https://github.com/brave/brave-browser/issues/22040 is implemented. */}
          {!isSolanaTxn &&
            <TransactionFeesTooltip
              text={
                <>
                  <TransactionFeeTooltipTitle>{getLocale('braveWalletAllowSpendTransactionFee')}</TransactionFeeTooltipTitle>
                  <TransactionFeeTooltipBody>
                    {transactionsNetwork && new Amount(transactionDetails.gasFee)
                      .divideByDecimals(transactionsNetwork.decimals)
                      .formatAsAsset(6, transactionsNetwork.symbol)
                    }
                  </TransactionFeeTooltipBody>
                  <TransactionFeeTooltipBody>
                    {
                      new Amount(transactionDetails.gasFeeFiat)
                        .formatAsFiat(defaultCurrencies.fiat)
                    }
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
              {[BraveWallet.TransactionStatus.Approved, BraveWallet.TransactionStatus.Submitted, BraveWallet.TransactionStatus.Confirmed, BraveWallet.TransactionStatus.Dropped].includes(transactionDetails.status) &&
                <>
                  <TransactionPopupItem
                    onClick={onClickViewOnBlockExplorer('tx', transaction.txHash)}
                    text={getLocale('braveWalletTransactionExplorer')}
                  />
                  <TransactionPopupItem
                    onClick={onClickCopyTransactionHash}
                    text={getLocale('braveWalletTransactionCopyHash')}
                  />
                </>
              }

              {[BraveWallet.TransactionStatus.Submitted, BraveWallet.TransactionStatus.Approved].includes(transactionDetails.status) &&
                !isSolanaTxn &&
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

              {[BraveWallet.TransactionStatus.Error].includes(transactionDetails.status) &&
                !isSolanaTxn &&
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
