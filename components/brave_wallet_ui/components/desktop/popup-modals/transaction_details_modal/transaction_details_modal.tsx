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
  useGetSolanaEstimatedFeeQuery,
  useRetryTransactionMutation,
  useSpeedupTransactionMutation
} from '../../../../common/slices/api.slice'

// Hooks
import { useExplorer } from '../../../../common/hooks/explorer'

// Types
import {
  BraveWallet,
  SerializableTransactionInfo,
  SpotPriceRegistry
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
  isFilecoinTransaction
} from '../../../../utils/tx-utils'
import { serializedTimeDeltaToJSDate } from '../../../../utils/datetime-utils'
import { getCoinFromTxDataUnion } from '../../../../utils/network-utils'
import { copyToClipboard } from '../../../../utils/copy-to-clipboard'
import { computeFiatAmount } from '../../../../utils/pricing-utils'
import Amount from '../../../../utils/amount'

// Components
import { PopupModal } from '../../popup-modals/index'
import { withPlaceholderIcon } from '../../../shared/create-placeholder-icon'
import { NftIcon } from '../../../shared/nft-icon/nft-icon'

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
  networkAsset?: BraveWallet.BlockchainToken
  sendToken?: BraveWallet.BlockchainToken
  buyToken?: BraveWallet.BlockchainToken
  sellToken?: BraveWallet.BlockchainToken
  txNetwork?: BraveWallet.NetworkInfo
  txTypeLocale: string
  formattedSendCurrencyTotal: string
  formattedSendFiatValue: string
  defaultFiatCurrency: string
  senderLabel: string
  recipient: string
  recipientLabel: string
  approvalTargetLabel: string
  formattedSellAmount: string
  formattedBuyAmount: string
  formattedBuyFiatValue: string
  spotPriceRegistry: SpotPriceRegistry | undefined
}

export const TransactionDetailsModal = (props: Props) => {
  const {
    onClose,
    transaction,
    sendToken,
    txNetwork,
    txTypeLocale,
    formattedSendCurrencyTotal,
    formattedSendFiatValue,
    defaultFiatCurrency,
    networkAsset,
    senderLabel,
    approvalTargetLabel,
    recipient,
    recipientLabel,
    buyToken,
    sellToken,
    formattedSellAmount,
    formattedBuyAmount,
    formattedBuyFiatValue,
    spotPriceRegistry
  } = props

  // Constants
  const txCoinType = getCoinFromTxDataUnion(transaction.txDataUnion)
  const isEthereumTx = isEthereumTransaction(transaction)
  const isSolanaTx = isSolanaTransaction(transaction)
  const isFilecoinTx = isFilecoinTransaction(transaction)
  const isSwapTx = isSwapTransaction(transaction)
  const isSolanaSwap = isSwapTx && isSolanaTx

  // Queries
  const { data: solFeeEstimates } = useGetSolanaEstimatedFeeQuery(
    isSolanaTx && transaction.chainId && transaction.id
      ? {
          chainId: transaction.chainId,
          txId: transaction.id
        }
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

  // Computed
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

  const { txStatus, fromAddress } = transaction

  const showCancelSpeedupButtons =
    isEthereumTx && cancelSpeedupTxTypes.includes(transaction.txStatus)

  const showRetryTransactionButton =
    BraveWallet.TransactionStatus.Error === transaction.txStatus &&
    !isSolanaTx &&
    !isFilecoinTx

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

  return (
    <PopupModal
      onClose={onClose}
      title='Transaction Details'
      width='630px'
      borderRadius={16}
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
              {!isSwapTx && (
                <>
                  {sendToken?.isNft ? (
                    <NFTIconWrapper width='unset'>
                      <NftIconWithPlaceholder
                        asset={sendToken}
                        network={txNetwork}
                        iconStyles={NftIconStyles}
                      />
                    </NFTIconWrapper>
                  ) : (
                    <Row
                      width='unset'
                      padding='0px 24px'
                    >
                      <AssetIconWithPlaceholder
                        asset={sendToken}
                        network={txNetwork}
                      />
                    </Row>
                  )}
                </>
              )}
              <TransactionValues
                padding={isSwapTx ? '0px 0px 0px 24px' : '0px'}
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
                {isSwapTx && (
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
                          <SwapIconWithPlaceholder
                            asset={sellToken}
                            network={txNetwork}
                          />
                          <SwapAmountText
                            textSize='14px'
                            isBold={true}
                            textAlign='left'
                          >
                            {formattedSellAmount}
                          </SwapAmountText>
                          <ArrowIcon />
                        </Row>
                        <Row
                          width='unset'
                          justifyContent='flex-start'
                        >
                          <SwapIconWithPlaceholder
                            asset={buyToken}
                            network={txNetwork}
                          />
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
                      </>
                    )}
                  </>
                )}
                {!isSwapTx && (
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
                      'tx',
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

        {showRetryTransactionButton && (
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
