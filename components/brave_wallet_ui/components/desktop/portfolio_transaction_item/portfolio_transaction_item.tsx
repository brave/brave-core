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

// Utils
import {
  formatDateAsRelative,
  serializedTimeDeltaToJSDate
} from '../../../utils/datetime-utils'
import { getPriceIdForToken } from '../../../utils/api-utils'
import { getLocale } from '../../../../common/locale'
import {
  getTransactionFormattedSendCurrencyTotal,
  isSolanaTransaction,
  getTransactionToAddress,
  findTransactionToken,
  getTransactionApprovalTargetAddress,
  isSwapTransaction,
  getETHSwapTransactionBuyAndSellTokens,
  getTransactionTransferredValue,
  getIsTxApprovalUnlimited
} from '../../../utils/tx-utils'
import {
  accountInfoEntityAdaptorInitialState //
} from '../../../common/slices/entities/account-info.entity'
import { makeNetworkAsset } from '../../../options/asset-options'
import { getCoinFromTxDataUnion } from '../../../utils/network-utils'
import { getAddressLabel, getAccountLabel } from '../../../utils/account-utils'
import { computeFiatAmount } from '../../../utils/pricing-utils'
import { isNativeAsset } from '../../../utils/asset-utils'
import Amount from '../../../utils/amount'

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
export interface Props {
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
    const { isSolanaTx, recipient, approvalTarget, isSwap, txCoinType } =
      React.useMemo(() => {
        return {
          isSolanaTx: isSolanaTransaction(transaction),
          recipient: getTransactionToAddress(transaction),
          approvalTarget: getTransactionApprovalTargetAddress(transaction),
          isSwap: isSwapTransaction(transaction),
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

    const { buyToken, sellToken, buyAmount, sellAmount } = React.useMemo(() => {
      return transaction.txType === BraveWallet.TransactionType.ETHSwap
        ? getETHSwapTransactionBuyAndSellTokens({
            nativeAsset: networkAsset,
            tokensList: combinedTokensList,
            tx: transaction
          })
        : {
            buyToken: undefined,
            sellToken: txToken,
            buyAmount: new Amount(''),
            sellAmount: new Amount(''),
            buyAmountWei: new Amount('')
          }
    }, [transaction, networkAsset, combinedTokensList, txToken])

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
        : isSwap
        ? 'currency-exchange'
        : 'send'

    const transactionTypeLocale =
      transaction.txType === BraveWallet.TransactionType.ERC20Approve
        ? 'braveWalletApprovalTransactionIntent'
        : isSwap
        ? 'braveWalletSwap'
        : 'braveWalletTransactionSent'

    const intentLabel =
      transaction.txType === BraveWallet.TransactionType.ERC20Approve || isSwap
        ? 'braveWalletOn'
        : 'braveWalletFrom'

    const intentAddress =
      transaction.txType === BraveWallet.TransactionType.ERC20Approve
        ? approvalTargetLabel
        : isSwap && !isSolanaTx
        ? recipientLabel
        : senderLabel

    const isNonFungibleToken = txToken?.isNft || txToken?.isErc721
    const formattedApprovalAmount = isTxApprovalUnlimited
      ? getLocale('braveWalletTransactionApproveUnlimited')
      : formattedSendCurrencyTotal
    const formattedSellAmount = sellAmount?.formatAsAsset(6, sellToken?.symbol)
    const formattedBuyAmount = buyAmount?.formatAsAsset(6, buyToken?.symbol)
    const isSolanaSwap = isSwap && isSolanaTx
    const showAmounts = !txToken?.isNft && !isSolanaSwap
    const showTransactionStatus = !noneTxStatusDisplayTypes.includes(
      transaction.txStatus
    )
    const nativeAssetWasSent = sendToken && isNativeAsset(sendToken)
    const showNetworkIcon = txNetwork && (!nativeAssetWasSent || isSwap)

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
                    {isSwap ? (
                      <SwapIconsWrapper>
                        <SwapSellIcon>
                          {isSolanaSwap ? (
                            <SellIconPlaceholder>
                              <SwapPlaceholderIcon />
                            </SellIconPlaceholder>
                          ) : (
                            <SwapIconWithPlaceholder
                              asset={sellToken}
                              network={txNetwork}
                            />
                          )}
                        </SwapSellIcon>
                        <SwapBuyIcon>
                          {isSolanaSwap ? (
                            <BuyIconPlaceholder>
                              <SwapPlaceholderIcon />
                            </BuyIconPlaceholder>
                          ) : (
                            <SwapIconWithPlaceholder
                              asset={buyToken}
                              network={txNetwork}
                            />
                          )}
                        </SwapBuyIcon>
                      </SwapIconsWrapper>
                    ) : (
                      <>
                        {isNonFungibleToken ? (
                          <NftIconWithPlaceholder
                            asset={sendToken}
                            network={txNetwork}
                          />
                        ) : (
                          <AssetIconWithPlaceholder
                            asset={sendToken}
                            network={txNetwork}
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
                {isSwap ? (
                  <Row>
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
                              {buyToken?.symbol ?? ''}
                            </TokenNameText>
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
                {isSwap ? (
                  <>
                    <FiatValueText
                      textSize='12px'
                      isBold={false}
                      textAlign='right'
                    >
                      {`-${formattedSellAmount}`}
                    </FiatValueText>
                    <AssetValueText
                      textSize='14px'
                      isBold={true}
                      textAlign='right'
                    >
                      {`+${formattedBuyAmount}`}
                    </AssetValueText>
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
