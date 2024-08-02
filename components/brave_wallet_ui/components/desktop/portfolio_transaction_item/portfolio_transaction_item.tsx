// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Types
import {
  BraveWallet,
  SerializableTransactionInfo
} from '../../../constants/types'

// Constants
import {
  LiFiExchangeProxy,
  SwapExchangeProxy
} from '../../../common/constants/registry'

// Utils
import {
  formatDateAsRelative,
  serializedTimeDeltaToJSDate
} from '../../../utils/datetime-utils'
import { getLocale } from '../../../../common/locale'
import {
  getTransactionFormattedSendCurrencyTotal,
  isSolanaTransaction,
  getTransactionToAddress,
  findTransactionToken,
  getTransactionApprovalTargetAddress,
  isSwapTransaction,
  getTransactionTransferredValue,
  getIsTxApprovalUnlimited,
  isBridgeTransaction
} from '../../../utils/tx-utils'
import {
  accountInfoEntityAdaptorInitialState //
} from '../../../common/slices/entities/account-info.entity'
import { makeNetworkAsset } from '../../../options/asset-options'
import { getCoinFromTxDataUnion } from '../../../utils/network-utils'
import { getAddressLabel, getAccountLabel } from '../../../utils/account-utils'
import {
  computeFiatAmount,
  getPriceIdForToken
} from '../../../utils/pricing-utils'
import { isNativeAsset } from '../../../utils/asset-utils'

// Hooks
import {
  useGetAccountInfosRegistryQuery,
  useGetDefaultFiatCurrencyQuery,
  useGetNetworkQuery,
  useGetTokenSpotPricesQuery
} from '../../../common/slices/api.slice'
import {
  useAccountQuery,
  useGetCombinedTokensListQuery
} from '../../../common/slices/api.slice.extra'
import { useSwapTransactionParser } from '../../../common/hooks/use-swap-tx-parser'

// Components
import { NftIcon } from '../../shared/nft-icon/nft-icon'
import { Skeleton } from '../../shared/loading-skeleton/styles'
import { withPlaceholderIcon } from '../../shared/create-placeholder-icon'
import { CreateNetworkIcon } from '../../shared/create-network-icon'

// Styled Components
import {
  PortfolioTransactionItemWrapper,
  TransactionTypeIcon,
  DateText,
  TransactionTypeText,
  IntentAddressText,
  AssetIcon,
  SwapIcon,
  SwapSellIcon,
  SwapBuyIcon,
  TokenNameText,
  TokenSymbolText,
  AssetValueText,
  FiatValueText,
  ArrowIcon,
  ArrowIconWrapper,
  StatusBubble,
  StatusIcon,
  LoadingIcon,
  BalancesColumn,
  NetworkIconWrapper,
  IconWrapper,
  SwapIconsWrapper,
  SellIconPlaceholder,
  BuyIconPlaceholder,
  SwapPlaceholderIcon
} from './portfolio_transaction_item.style'
import { Column, Row, VerticalSpace } from '../../shared/style'

const noneTxStatusDisplayTypes = [
  BraveWallet.TransactionStatus.Approved,
  BraveWallet.TransactionStatus.Confirmed,
  BraveWallet.TransactionStatus.Signed
]
interface Props {
  transaction: BraveWallet.TransactionInfo | SerializableTransactionInfo
  isFocused?: boolean
  onClick?: (
    tx: Pick<BraveWallet.TransactionInfo | SerializableTransactionInfo, 'id'>
  ) => void
}

const ICON_ASSET_CONFIG = {
  size: 'medium',
  marginLeft: 0,
  marginRight: 0
} as const
const ICON_SWAP_CONFIG = {
  size: 'small',
  marginLeft: 0,
  marginRight: 0
} as const
const AssetIconWithPlaceholder = withPlaceholderIcon(
  AssetIcon,
  ICON_ASSET_CONFIG
)
const SwapIconWithPlaceholder = withPlaceholderIcon(SwapIcon, ICON_SWAP_CONFIG)
const NftIconWithPlaceholder = withPlaceholderIcon(NftIcon, ICON_ASSET_CONFIG)

export const PortfolioTransactionItem = React.forwardRef<HTMLDivElement, Props>(
  ({ transaction, isFocused, onClick }: Props, forwardedRef) => {
    // partial tx parsing
    const {
      isSolanaTx,
      recipient,
      approvalTarget,
      isSwap,
      isBridge,
      txCoinType
    } = React.useMemo(() => {
      return {
        isSolanaTx: isSolanaTransaction(transaction),
        recipient: getTransactionToAddress(transaction),
        approvalTarget: getTransactionApprovalTargetAddress(transaction),
        isSwap: isSwapTransaction(transaction),
        isBridge: isBridgeTransaction(transaction),
        txCoinType: getCoinFromTxDataUnion(transaction.txDataUnion)
      }
    }, [transaction])

    // Queries
    const {
      data: defaultFiatCurrency = '',
      isLoading: isLoadingDefaultFiatCurrency
    } = useGetDefaultFiatCurrencyQuery(undefined)

    const { data: txNetwork } = useGetNetworkQuery({
      chainId: transaction.chainId,
      coin: txCoinType
    })

    const { data: toNetwork } = useGetNetworkQuery(
      isBridge &&
        transaction.swapInfo?.toChainId &&
        transaction.swapInfo.toCoin !== undefined
        ? {
            chainId: transaction.swapInfo.toChainId,
            coin: transaction.swapInfo.toCoin
          }
        : skipToken
    )

    const { data: combinedTokensList, isLoading: isLoadingTokens } =
      useGetCombinedTokensListQuery()

    const {
      data: accountInfosRegistry = accountInfoEntityAdaptorInitialState
    } = useGetAccountInfosRegistryQuery(undefined)

    const { account } = useAccountQuery(transaction.fromAccountId)

    // memos & computed from queries
    const networkAsset = React.useMemo(() => {
      return makeNetworkAsset(txNetwork)
    }, [txNetwork])

    const txToken = findTransactionToken(transaction, combinedTokensList)

    const recipientLabel = getAddressLabel(recipient, accountInfosRegistry)

    const senderLabel = getAccountLabel(
      transaction.fromAccountId,
      accountInfosRegistry
    )

    const approvalTargetLabel = getAddressLabel(
      approvalTarget,
      accountInfosRegistry
    )

    const { buyToken, sellToken, buyAmountWei, sellAmountWei } =
      useSwapTransactionParser(transaction)

    const [normalizedTransferredValue, transferredValueWei] =
      React.useMemo(() => {
        const { normalized, wei } = getTransactionTransferredValue({
          tx: transaction,
          sellToken,
          token: txToken,
          txAccount: account,
          txNetwork
        })
        return [normalized.format(6), wei]
      }, [transaction, sellToken, txToken, account, txNetwork])

    const formattedSendCurrencyTotal = getTransactionFormattedSendCurrencyTotal(
      {
        normalizedTransferredValue,
        tx: transaction,
        sellToken,
        token: txToken,
        txNetwork
      }
    )

    const networkAssetPriceId = networkAsset
      ? getPriceIdForToken(networkAsset)
      : ''
    const txTokenPriceId = txToken ? getPriceIdForToken(txToken) : ''
    const sellTokenPriceId = sellToken ? getPriceIdForToken(sellToken) : ''
    const buyTokenPriceId = buyToken ? getPriceIdForToken(buyToken) : ''
    const priceIds = [
      networkAssetPriceId,
      txTokenPriceId,
      sellTokenPriceId,
      buyTokenPriceId
    ].filter(Boolean)

    // price queries
    const { data: spotPriceRegistry, isLoading: isLoadingTxTokenSpotPrice } =
      useGetTokenSpotPricesQuery(
        priceIds.length && defaultFiatCurrency
          ? { ids: priceIds, toCurrency: defaultFiatCurrency }
          : skipToken
      )

    // Computed
    const isSwapOrBridge = isSwap || isBridge
    const sendToken =
      transaction.txType === BraveWallet.TransactionType.ETHSend ||
      transaction.fromAccountId.coin === BraveWallet.CoinType.FIL ||
      transaction.fromAccountId.coin === BraveWallet.CoinType.BTC ||
      transaction.txType === BraveWallet.TransactionType.SolanaSystemTransfer
        ? networkAsset
        : txToken

    const computedSendFiatAmount = sendToken
      ? computeFiatAmount({
          spotPriceRegistry,
          value: transferredValueWei.format(),
          token: sendToken
        }).formatAsFiat(defaultFiatCurrency)
      : ''

    const isTxApprovalUnlimited = getIsTxApprovalUnlimited(transaction)

    const formattedSendFiatValue =
      transaction.txType === BraveWallet.TransactionType.ERC20Approve
        ? isTxApprovalUnlimited
          ? getLocale('braveWalletTransactionApproveUnlimited')
          : computedSendFiatAmount
        : computedSendFiatAmount

    const transactionTypeIcon =
      transaction.txType === BraveWallet.TransactionType.ERC20Approve
        ? 'check-normal'
        : isSwapOrBridge
        ? 'currency-exchange'
        : 'send'

    const transactionTypeLocale =
      transaction.txType === BraveWallet.TransactionType.ERC20Approve
        ? 'braveWalletApprovalTransactionIntent'
        : isBridge
        ? 'braveWalletBridge'
        : isSwap
        ? 'braveWalletSwap'
        : 'braveWalletTransactionSent'

    const intentLabel =
      transaction.txType === BraveWallet.TransactionType.ERC20Approve ||
      isSwapOrBridge
        ? 'braveWalletOn'
        : 'braveWalletFrom'

    const intentAddress =
      transaction.txType === BraveWallet.TransactionType.ERC20Approve
        ? approvalTargetLabel
        : (isSwapOrBridge &&
            (recipient.toLowerCase() === SwapExchangeProxy ||
              recipient.toLowerCase() === LiFiExchangeProxy)) ||
          (isSwap && !isSolanaTx)
        ? recipientLabel
        : senderLabel

    const isNonFungibleToken = txToken?.isNft || txToken?.isErc721
    const formattedApprovalAmount = isTxApprovalUnlimited
      ? getLocale('braveWalletTransactionApproveUnlimited')
      : formattedSendCurrencyTotal
    const formattedSellAmount = sellToken
      ? sellAmountWei
          .divideByDecimals(sellToken.decimals)
          .formatAsAsset(6, sellToken.symbol)
      : ''
    const formattedBuyAmount = buyToken
      ? buyAmountWei
          .divideByDecimals(buyToken.decimals)
          .formatAsAsset(6, buyToken.symbol)
      : ''
    const isSolanaSwap = isSwap && isSolanaTx
    const showAmounts = !txToken?.isNft && !isSolanaSwap
    const showTransactionStatus = !noneTxStatusDisplayTypes.includes(
      transaction.txStatus
    )
    const nativeAssetWasSent = sendToken && isNativeAsset(sendToken)
    const showNetworkIcon = txNetwork && (!nativeAssetWasSent || isSwapOrBridge)

    // render
    return (
      <PortfolioTransactionItemWrapper
        ref={forwardedRef}
        isFocused={isFocused}
        onClick={() => onClick?.(transaction)}
      >
        <Column fullWidth={true}>
          <Row
            justifyContent='flex-start'
            padding='2px'
            margin='0px 0px 4px 0px'
          >
            <DateText
              textSize='12px'
              isBold={false}
            >
              {formatDateAsRelative(
                serializedTimeDeltaToJSDate(transaction.createdTime)
              )}
            </DateText>
            <TransactionTypeIcon name={transactionTypeIcon} />
            <TransactionTypeText
              textSize='12px'
              isBold={false}
            >
              {getLocale(transactionTypeLocale)}
              {` `}
              {getLocale(intentLabel)}
            </TransactionTypeText>
            <IntentAddressText
              textSize='12px'
              isBold={true}
            >
              {intentAddress}
            </IntentAddressText>
          </Row>
          <Row justifyContent='space-between'>
            <Row
              justifyContent='flex-start'
              padding='5px 0px'
              width='unset'
            >
              <IconWrapper margin='0px 12px 0px 0px'>
                {isLoadingTokens ? (
                  <>
                    <Skeleton
                      width={32}
                      height={32}
                      circle={true}
                      enableAnimation={true}
                    />
                  </>
                ) : (
                  <>
                    {isSwap && !isBridge ? (
                      <SwapIconsWrapper>
                        <SwapSellIcon>
                          {isSolanaSwap ? (
                            <SellIconPlaceholder>
                              <SwapPlaceholderIcon />
                            </SellIconPlaceholder>
                          ) : (
                            <SwapIconWithPlaceholder asset={sellToken} />
                          )}
                        </SwapSellIcon>
                        <SwapBuyIcon>
                          {isSolanaSwap ? (
                            <BuyIconPlaceholder>
                              <SwapPlaceholderIcon />
                            </BuyIconPlaceholder>
                          ) : (
                            <SwapIconWithPlaceholder asset={buyToken} />
                          )}
                        </SwapBuyIcon>
                      </SwapIconsWrapper>
                    ) : (
                      <>
                        {isNonFungibleToken ? (
                          <NftIconWithPlaceholder asset={sendToken} />
                        ) : (
                          <AssetIconWithPlaceholder
                            asset={isBridge ? sellToken : sendToken}
                          />
                        )}
                      </>
                    )}
                  </>
                )}
                {showTransactionStatus && (
                  <StatusBubble status={transaction.txStatus}>
                    {[
                      BraveWallet.TransactionStatus.Submitted,
                      BraveWallet.TransactionStatus.Unapproved
                    ].includes(transaction.txStatus) ? (
                      <LoadingIcon />
                    ) : (
                      <StatusIcon name='loading-spinner' />
                    )}
                  </StatusBubble>
                )}
                {showNetworkIcon && (
                  <NetworkIconWrapper>
                    <CreateNetworkIcon
                      network={txNetwork}
                      marginRight={0}
                      size='small'
                    />
                  </NetworkIconWrapper>
                )}
              </IconWrapper>
              <Column alignItems='flex-start'>
                {isSwapOrBridge ? (
                  <Row gap='8px'>
                    {isSolanaSwap ? (
                      <TokenNameText
                        textSize='14px'
                        isBold={true}
                        textAlign='left'
                      >
                        {getLocale('braveWalletSolanaSwap')}
                      </TokenNameText>
                    ) : (
                      <>
                        {isLoadingTokens ? (
                          <>
                            <Skeleton
                              width={40}
                              height={20}
                              enableAnimation={true}
                            />
                            <ArrowIconWrapper>
                              <ArrowIcon />
                            </ArrowIconWrapper>
                            <Skeleton
                              width={40}
                              height={20}
                              enableAnimation={true}
                            />
                          </>
                        ) : (
                          <>
                            <TokenNameText
                              textSize='14px'
                              isBold={true}
                              textAlign='left'
                            >
                              {sellToken?.symbol ?? ''}
                            </TokenNameText>
                            <ArrowIconWrapper>
                              <ArrowIcon />
                            </ArrowIconWrapper>
                            <TokenNameText
                              textSize='14px'
                              isBold={true}
                              textAlign='left'
                            >
                              {isBridge && toNetwork
                                ? getLocale('braveWalletOnNetwork').replace(
                                    '$1',
                                    toNetwork.chainName
                                  )
                                : buyToken?.symbol ?? ''}
                            </TokenNameText>
                            {isBridge && toNetwork && (
                              <CreateNetworkIcon
                                network={toNetwork}
                                marginRight={0}
                                size='small'
                              />
                            )}
                          </>
                        )}
                      </>
                    )}
                  </Row>
                ) : (
                  <>
                    {isLoadingTokens ? (
                      <>
                        <Skeleton
                          height={20}
                          width={100}
                          enableAnimation={true}
                        />
                        <VerticalSpace space='4px' />
                        <Skeleton
                          height={16}
                          width={60}
                          enableAnimation={true}
                        />
                      </>
                    ) : (
                      <>
                        <TokenNameText
                          textSize='14px'
                          isBold={true}
                          textAlign='left'
                        >
                          {sendToken?.name ?? ''}
                        </TokenNameText>
                        <TokenSymbolText
                          textSize='12px'
                          isBold={false}
                          textAlign='left'
                        >
                          {sendToken?.symbol ?? ''}
                        </TokenSymbolText>
                      </>
                    )}
                  </>
                )}
              </Column>
            </Row>
            {showAmounts && (
              <BalancesColumn
                width='unset'
                alignItems='flex-end'
              >
                {isSwapOrBridge ? (
                  <>
                    {formattedSellAmount && (
                      <FiatValueText
                        textSize='12px'
                        isBold={false}
                        textAlign='right'
                      >
                        {`-${formattedSellAmount}`}
                      </FiatValueText>
                    )}
                    {formattedBuyAmount && (
                      <AssetValueText
                        textSize='14px'
                        isBold={true}
                        textAlign='right'
                      >
                        {`+${formattedBuyAmount}`}
                      </AssetValueText>
                    )}
                  </>
                ) : (
                  <>
                    <AssetValueText
                      textSize='14px'
                      isBold={true}
                      textAlign='right'
                    >
                      {transaction.txType ===
                      BraveWallet.TransactionType.ERC20Approve
                        ? formattedApprovalAmount
                        : `-${formattedSendCurrencyTotal}`}
                    </AssetValueText>
                    {isLoadingTxTokenSpotPrice &&
                    isLoadingDefaultFiatCurrency ? (
                      <Skeleton
                        width={60}
                        height={18}
                        enableAnimation={true}
                      />
                    ) : (
                      <FiatValueText
                        textSize='12px'
                        isBold={false}
                        textAlign='right'
                      >
                        {formattedSendFiatValue}
                      </FiatValueText>
                    )}
                  </>
                )}
              </BalancesColumn>
            )}
          </Row>
        </Column>
      </PortfolioTransactionItemWrapper>
    )
  }
)

export default PortfolioTransactionItem
