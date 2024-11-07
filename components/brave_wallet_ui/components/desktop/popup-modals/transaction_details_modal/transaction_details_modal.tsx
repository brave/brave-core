// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

// Slices
import {
  useCancelTransactionMutation,
  useGetAccountInfosRegistryQuery,
  useGetDefaultFiatCurrencyQuery,
  useGetNetworkQuery,
  useGetSolanaEstimatedFeeQuery,
  useGetTokenSpotPricesQuery,
  useRetryTransactionMutation,
  useSpeedupTransactionMutation
} from '../../../../common/slices/api.slice'
import {
  useGetCombinedTokensListQuery,
  useAccountQuery
} from '../../../../common/slices/api.slice.extra'
import {
  accountInfoEntityAdaptorInitialState //
} from '../../../../common/slices/entities/account-info.entity'

// Hooks
import { useExplorer } from '../../../../common/hooks/explorer'
import { useSwapTransactionParser } from '../../../../common/hooks/use-swap-tx-parser'

// Types
import {
  BraveWallet,
  SerializableTransactionInfo
} from '../../../../constants/types'

// Utils
import { getLocale } from '../../../../../common/locale'
import {
  getTransactionGasFee,
  getTransactionStatusString,
  getIsTxApprovalUnlimited,
  isSwapTransaction,
  isEthereumTransaction,
  isSolanaTransaction,
  getTransactionToAddress,
  getTransactionApprovalTargetAddress,
  getTransactionTransferredValue,
  getTransactionFormattedSendCurrencyTotal,
  findTransactionToken,
  isBridgeTransaction
} from '../../../../utils/tx-utils'
import { serializedTimeDeltaToJSDate } from '../../../../utils/datetime-utils'
import { getCoinFromTxDataUnion } from '../../../../utils/network-utils'
import { copyToClipboard } from '../../../../utils/copy-to-clipboard'
import {
  computeFiatAmount,
  getPriceIdForToken
} from '../../../../utils/pricing-utils'
import Amount from '../../../../utils/amount'
import {
  getAddressLabel,
  getAccountLabel
} from '../../../../utils/account-utils'
import { makeNetworkAsset } from '../../../../options/asset-options'

// Components
import { PopupModal } from '../../popup-modals/index'
import { withPlaceholderIcon } from '../../../shared/create-placeholder-icon'
import { NftIcon } from '../../../shared/nft-icon/nft-icon'
import { CreateNetworkIcon } from '../../../shared/create-network-icon'

// Styled Components
import {
  ContentWrapper,
  HeroContainer,
  HeroContent,
  SectionRow,
  SectionLabel,
  SectionInfoText,
  HeroBackground,
  AssetIcon,
  TransactionTypeText,
  TransactionTotalText,
  TransactionFiatText,
  StatusBox,
  StatusText,
  LoadingIcon,
  ErrorIcon,
  SuccessIcon,
  DateText,
  NetworkNameText,
  SpeedupIcon,
  RetryIcon,
  IntentAddressText,
  NftIconStyles,
  SwapIcon,
  SwapAmountText,
  ArrowIcon,
  SwapFiatValueText,
  RowWrapped,
  IconAndValue,
  TransactionValues,
  StatusBoxWrapper,
  NFTIconWrapper
} from './transaction_details_modal.style'
import {
  SellIconPlaceholder,
  BuyIconPlaceholder,
  SwapPlaceholderIcon
} from '../../portfolio_transaction_item/portfolio_transaction_item.style'
import {
  Column,
  HorizontalSpace,
  Row,
  VerticalDivider,
  VerticalSpace
} from '../../../shared/style'

const ICON_ASSET_CONFIG = {
  size: 'big',
  marginLeft: 0,
  marginRight: 0
} as const
const AssetIconWithPlaceholder = withPlaceholderIcon(
  AssetIcon,
  ICON_ASSET_CONFIG
)

const ICON_SWAP_CONFIG = {
  size: 'small',
  marginLeft: 0,
  marginRight: 8
} as const
const SwapIconWithPlaceholder = withPlaceholderIcon(SwapIcon, ICON_SWAP_CONFIG)

const NftIconWithPlaceholder = withPlaceholderIcon(NftIcon, ICON_ASSET_CONFIG)

const cancelSpeedupTxTypes = [
  BraveWallet.TransactionStatus.Submitted,
  BraveWallet.TransactionStatus.Approved
]

const pendingTxTypes = [
  BraveWallet.TransactionStatus.Submitted,
  BraveWallet.TransactionStatus.Unapproved
]

const successTxTypes = [
  BraveWallet.TransactionStatus.Approved,
  BraveWallet.TransactionStatus.Confirmed,
  BraveWallet.TransactionStatus.Signed
]

const errorTxTypes = [
  BraveWallet.TransactionStatus.Error,
  BraveWallet.TransactionStatus.Dropped,
  BraveWallet.TransactionStatus.Rejected
]

interface Props {
  onClose: () => void
  transaction: BraveWallet.TransactionInfo | SerializableTransactionInfo
}

export const TransactionDetailsModal = ({ onClose, transaction }: Props) => {
  // Constants
  const txCoinType = getCoinFromTxDataUnion(transaction.txDataUnion)
  const isEthereumTx = isEthereumTransaction(transaction)
  const isSolanaTx = isSolanaTransaction(transaction)
  const isSwapTx = isSwapTransaction(transaction)
  const isBridgeTx = isBridgeTransaction(transaction)
  const isSolanaSwap = isSwapTx && isSolanaTx
  const recipient = getTransactionToAddress(transaction)
  const approvalTarget = getTransactionApprovalTargetAddress(transaction)

  // Queries
  const { data: defaultFiatCurrency = '' } =
    useGetDefaultFiatCurrencyQuery(undefined)
  const { data: txNetwork } = useGetNetworkQuery({
    chainId: transaction.chainId,
    coin: txCoinType
  })

  const { data: toNetwork } = useGetNetworkQuery(
    isBridgeTx &&
      transaction.swapInfo?.toChainId &&
      transaction.swapInfo.toCoin !== undefined
      ? {
          chainId: transaction.swapInfo.toChainId,
          coin: transaction.swapInfo.toCoin
        }
      : skipToken
  )

  const { data: solFeeEstimates } = useGetSolanaEstimatedFeeQuery(
    isSolanaTx && transaction.chainId && transaction.id
      ? {
          chainId: transaction.chainId,
          txId: transaction.id
        }
      : skipToken
  )
  const { data: combinedTokensList } = useGetCombinedTokensListQuery()

  const { data: accountInfosRegistry = accountInfoEntityAdaptorInitialState } =
    useGetAccountInfosRegistryQuery(undefined)

  const { account } = useAccountQuery(transaction.fromAccountId)

  // memos & computed from queries
  const networkAsset = React.useMemo(() => {
    return makeNetworkAsset(txNetwork)
  }, [txNetwork])

  const txToken = findTransactionToken(transaction, combinedTokensList)

  const { buyToken, sellToken, buyAmountWei, sellAmountWei } =
    useSwapTransactionParser(transaction)

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
  const { data: spotPriceRegistry } = useGetTokenSpotPricesQuery(
    priceIds.length && defaultFiatCurrency
      ? { ids: priceIds, toCurrency: defaultFiatCurrency }
      : skipToken
  )

  // Hooks
  const onClickViewOnBlockExplorer = useExplorer(txNetwork)
  const [speedupTx] = useSpeedupTransactionMutation()
  const [retryTx] = useRetryTransactionMutation()
  const [cancelTx] = useCancelTransactionMutation()

  // Methods
  const onClickRetryTransaction = () => {
    retryTx({
      coinType: txCoinType,
      chainId: transaction.chainId,
      transactionId: transaction.id
    })
  }

  const onClickSpeedupTransaction = () => {
    speedupTx({
      coinType: txCoinType,
      chainId: transaction.chainId,
      transactionId: transaction.id
    })
  }

  const onClickCancelTransaction = () => {
    cancelTx({
      coinType: txCoinType,
      chainId: transaction.chainId,
      transactionId: transaction.id
    })
  }

  // memos
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

  const formattedSendCurrencyTotal = getTransactionFormattedSendCurrencyTotal({
    normalizedTransferredValue,
    tx: transaction,
    sellToken,
    token: txToken,
    txNetwork
  })

  // Computed
  const isBridgeOrSwap = isSwapTx || isBridgeTx
  const sendToken =
    transaction.txType === BraveWallet.TransactionType.ETHSend ||
    transaction.fromAccountId.coin === BraveWallet.CoinType.FIL ||
    transaction.fromAccountId.coin === BraveWallet.CoinType.BTC ||
    transaction.txType === BraveWallet.TransactionType.SolanaSystemTransfer
      ? networkAsset
      : txToken

  const formattedBuyFiatValue = buyToken
    ? computeFiatAmount({
        spotPriceRegistry,
        value: buyAmountWei.format(),
        token: buyToken
      }).formatAsFiat(defaultFiatCurrency)
    : ''

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

  const txTypeLocale =
    transaction.txType === BraveWallet.TransactionType.ERC20Approve
      ? 'braveWalletApprovalTransactionIntent'
      : isBridgeTx
      ? 'braveWalletBridge'
      : isSwapTx
      ? 'braveWalletSwap'
      : 'braveWalletTransactionSent'

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

  const gasFee =
    txCoinType === BraveWallet.CoinType.SOL
      ? solFeeEstimates ?? ''
      : getTransactionGasFee(transaction)

  const formattedGasFeeFiatValue =
    networkAsset && spotPriceRegistry
      ? computeFiatAmount({
          spotPriceRegistry,
          value: gasFee,
          token: networkAsset
        }).formatAsFiat(defaultFiatCurrency)
      : ''

  const { txStatus, fromAddress, isRetriable } = transaction

  const showCancelSpeedupButtons =
    isEthereumTx && cancelSpeedupTxTypes.includes(transaction.txStatus)

  const txCurrencyTotal =
    transaction.txType === BraveWallet.TransactionType.ERC20Approve
      ? getIsTxApprovalUnlimited(transaction)
        ? `${getLocale('braveWalletTransactionApproveUnlimited')} ${
            //
            sendToken?.symbol ?? ''
          }`
        : formattedSendCurrencyTotal
      : sendToken?.isNft
      ? sendToken.name
      : formattedSendCurrencyTotal

  const txFiatTotal = sendToken?.isNft
    ? sendToken.symbol
    : formattedSendFiatValue

  const showPendingTxStatus = pendingTxTypes.includes(txStatus)

  const showSuccessTxStatus = successTxTypes.includes(txStatus)

  const showErrorTxStatus = errorTxTypes.includes(txStatus)

  const recipientLabel = getAddressLabel(recipient, accountInfosRegistry)

  const senderLabel = getAccountLabel(
    transaction.fromAccountId,
    accountInfosRegistry
  )

  const approvalTargetLabel = getAddressLabel(
    approvalTarget,
    accountInfosRegistry
  )

  const memoFromTransaction = transaction.txDataUnion.zecTxData?.memo

  const memoText = String.fromCharCode(
    ...memoFromTransaction ?? []
  )

  // render
  return (
    <PopupModal
      onClose={onClose}
      title='Transaction Details'
      width='630px'
    >
      <ContentWrapper
        fullWidth={true}
        fullHeight={true}
        justifyContent='flex-start'
      >
        <HeroContainer width='100%'>
          <HeroBackground />
          <HeroContent
            width='100%'
            justifyContent='space-between'
          >
            <IconAndValue width='unset'>
              {!isBridgeOrSwap && (
                <>
                  {sendToken?.isNft ? (
                    <NFTIconWrapper width='unset'>
                      <NftIconWithPlaceholder
                        asset={sendToken}
                        iconStyles={NftIconStyles}
                      />
                    </NFTIconWrapper>
                  ) : (
                    <Row
                      width='unset'
                      padding='0px 24px'
                    >
                      <AssetIconWithPlaceholder asset={sendToken} />
                    </Row>
                  )}
                </>
              )}
              <TransactionValues
                padding={isBridgeOrSwap ? '0px 0px 0px 24px' : '0px'}
              >
                <TransactionTypeText
                  isBold={false}
                  textSize='16px'
                  textAlign='left'
                >
                  {isSolanaSwap
                    ? getLocale('braveWalletSolanaSwap')
                    : getLocale(txTypeLocale)}
                </TransactionTypeText>
                {isBridgeOrSwap && (
                  <>
                    {isSolanaSwap ? (
                      <Row
                        width='unset'
                        margin='0px 0px 10px 0px'
                      >
                        <SellIconPlaceholder>
                          <SwapPlaceholderIcon />
                        </SellIconPlaceholder>
                        <ArrowIcon />
                        <BuyIconPlaceholder>
                          <SwapPlaceholderIcon />
                        </BuyIconPlaceholder>
                      </Row>
                    ) : (
                      <>
                        <VerticalSpace space='4px' />
                        <Row
                          width='unset'
                          margin='0px 0px 10px 0px'
                        >
                          <SwapIconWithPlaceholder asset={sellToken} />
                          <SwapAmountText
                            textSize='14px'
                            isBold={true}
                            textAlign='left'
                          >
                            {formattedSellAmount}
                          </SwapAmountText>
                          <ArrowIcon />
                        </Row>
                        {isBridgeTx ? (
                          <Row
                            width='unset'
                            justifyContent='flex-start'
                            gap='4px'
                          >
                            <SwapAmountText
                              textSize='14px'
                              isBold={true}
                              textAlign='left'
                            >
                              {getLocale('braveWalletOnNetwork').replace(
                                '$1',
                                toNetwork?.chainName ?? ''
                              )}
                            </SwapAmountText>
                            {toNetwork && (
                              <CreateNetworkIcon
                                network={toNetwork}
                                marginRight={0}
                                size='small'
                              />
                            )}
                          </Row>
                        ) : (
                          <Row
                            width='unset'
                            justifyContent='flex-start'
                          >
                            <SwapIconWithPlaceholder asset={buyToken} />
                            <RowWrapped
                              width='unset'
                              justifyContent='flex-start'
                            >
                              <SwapAmountText
                                textSize='14px'
                                isBold={true}
                                textAlign='left'
                              >
                                {formattedBuyAmount}
                              </SwapAmountText>
                              <SwapFiatValueText
                                textSize='14px'
                                textAlign='left'
                              >
                                {`(${formattedBuyFiatValue})`}
                              </SwapFiatValueText>
                            </RowWrapped>
                          </Row>
                        )}
                      </>
                    )}
                  </>
                )}
                {!isBridgeOrSwap && (
                  <>
                    <TransactionTotalText
                      isBold={true}
                      textSize='16px'
                      textAlign='left'
                    >
                      {txCurrencyTotal}
                    </TransactionTotalText>
                    {transaction.txType !==
                      BraveWallet.TransactionType.ERC20Approve && (
                      <TransactionFiatText
                        isBold={false}
                        textSize='12px'
                        textAlign='left'
                      >
                        {txFiatTotal}
                      </TransactionFiatText>
                    )}
                  </>
                )}
              </TransactionValues>
            </IconAndValue>
            <StatusBoxWrapper alignItems='flex-end'>
              <StatusBox status={txStatus}>
                {showPendingTxStatus && <LoadingIcon status={txStatus} />}
                {showSuccessTxStatus && <SuccessIcon />}
                {showErrorTxStatus && <ErrorIcon />}
                <StatusText
                  status={txStatus}
                  isBold={true}
                  textAlign='right'
                >
                  {getTransactionStatusString(txStatus)}
                </StatusText>
              </StatusBox>
              <DateText
                textSize='12px'
                textAlign='right'
              >
                {serializedTimeDeltaToJSDate(
                  transaction.createdTime
                ).toUTCString()}
              </DateText>
              <NetworkNameText
                textSize='12px'
                textAlign='right'
              >
                {txNetwork?.chainName ?? ''}
              </NetworkNameText>
            </StatusBoxWrapper>
          </HeroContent>
        </HeroContainer>

        {memoText && (
          <>
            <SectionRow padding='16px 0px'>
              <SectionLabel
                textAlign='left'
                textSize='14px'
              >
                {getLocale('braveWalletMemo')}
              </SectionLabel>
              <SectionInfoText
                textAlign='left'
                textSize='14px'
              >
                {memoText}
              </SectionInfoText>
            </SectionRow>
            <VerticalDivider />
          </>
        )}

        {transaction.txHash && (
          <>
            <SectionRow padding='16px 0px'>
              <SectionLabel
                textAlign='left'
                textSize='14px'
              >
                {getLocale('braveWalletTransactionDetailHash')}
              </SectionLabel>
              <Row justifyContent='space-between'>
                <SectionInfoText
                  textAlign='left'
                  textSize='14px'
                >
                  {transaction.txHash}
                </SectionInfoText>
                <Row width='unset'>
                  <HorizontalSpace space='12px' />
                  <Button
                    onClick={() => copyToClipboard(transaction.txHash)}
                    kind='outline'
                    size='tiny'
                    fab
                  >
                    <Icon name='copy' />
                  </Button>
                  <HorizontalSpace space='12px' />
                  <Button
                    onClick={onClickViewOnBlockExplorer(
                      transaction.swapInfo?.provider === 'lifi' ? 'lifi' : 'tx',
                      transaction.txHash
                    )}
                    kind='outline'
                    size='tiny'
                    fab
                  >
                    <Icon name='launch' />
                  </Button>
                </Row>
              </Row>
            </SectionRow>
            <VerticalDivider />
          </>
        )}

        <SectionRow padding='16px 0px'>
          <SectionLabel
            textAlign='left'
            textSize='14px'
          >
            {getLocale('braveWalletFrom')}
          </SectionLabel>
          <Row justifyContent='space-between'>
            <Column alignItems='flex-start'>
              {fromAddress && (
                <SectionInfoText
                  textAlign='left'
                  textSize='14px'
                >
                  {fromAddress}
                </SectionInfoText>
              )}
              <IntentAddressText
                textAlign='left'
                textSize='12px'
              >
                {senderLabel}
              </IntentAddressText>
            </Column>
            {fromAddress && (
              <Row width='unset'>
                <HorizontalSpace space='12px' />
                <Button
                  onClick={() => copyToClipboard(fromAddress)}
                  kind='outline'
                  size='tiny'
                  fab
                >
                  <Icon name='copy' />
                </Button>
              </Row>
            )}
          </Row>
        </SectionRow>
        <VerticalDivider />

        {recipient && (
          <>
            <SectionRow padding='16px 0px'>
              <SectionLabel
                textAlign='left'
                textSize='14px'
              >
                {getLocale('braveWalletSwapTo')}
              </SectionLabel>
              <Row justifyContent='space-between'>
                <Column alignItems='flex-start'>
                  <SectionInfoText
                    textAlign='left'
                    textSize='14px'
                  >
                    {recipient}
                  </SectionInfoText>
                  <IntentAddressText
                    textAlign='left'
                    textSize='12px'
                  >
                    {transaction.txType ===
                    BraveWallet.TransactionType.ERC20Approve
                      ? approvalTargetLabel
                      : recipientLabel}
                  </IntentAddressText>
                </Column>
                <Row width='unset'>
                  <HorizontalSpace space='12px' />
                  <Button
                    onClick={() =>
                      copyToClipboard(transaction?.effectiveRecipient ?? '')
                    }
                    kind='outline'
                    size='tiny'
                    fab
                  >
                    <Icon name='copy' />
                  </Button>
                </Row>
              </Row>
            </SectionRow>
            <VerticalDivider />
          </>
        )}

        <SectionRow padding='16px 0px 0px 0px'>
          <SectionLabel
            textAlign='left'
            textSize='14px'
          >
            {getLocale('braveWalletConfirmTransactionTransactionFee')}
          </SectionLabel>
          <Row width='unset'>
            <SectionInfoText
              textAlign='left'
              textSize='14px'
            >
              {txNetwork &&
                new Amount(gasFee)
                  .divideByDecimals(txNetwork.decimals)
                  .formatAsAsset(6, txNetwork.symbol)}{' '}
              (
              {
                //
                formattedGasFeeFiatValue
              }
              )
            </SectionInfoText>
          </Row>
        </SectionRow>

        {showCancelSpeedupButtons && (
          <Row padding='32px 0px 0px 0px'>
            <Row>
              <Button
                onClick={onClickCancelTransaction}
                kind='outline'
              >
                {getLocale('braveWalletTransactionCancel')}
              </Button>
            </Row>
            <HorizontalSpace space='16px' />
            <Row>
              <Button onClick={onClickSpeedupTransaction}>
                <Row>
                  <SpeedupIcon />
                  {getLocale('braveWalletTransactionSpeedup')}
                </Row>
              </Button>
            </Row>
          </Row>
        )}

        {isRetriable && (
          <Row padding='32px 0px 0px 0px'>
            <Button
              onClick={onClickRetryTransaction}
              kind='outline'
            >
              <Row>
                <RetryIcon />
                {getLocale('braveWalletTransactionRetry')}
              </Row>
            </Button>
          </Row>
        )}
      </ContentWrapper>
    </PopupModal>
  )
}
