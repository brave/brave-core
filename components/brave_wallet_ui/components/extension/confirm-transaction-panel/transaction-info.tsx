// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet } from '../../../constants/types'
import { ParsedTransaction } from '../../../utils/tx-utils'

// utils
import Amount from '../../../utils/amount'
import { getLocale } from '../../../../common/locale'

// hooks
import {
  useGetDefaultFiatCurrencyQuery, //
} from '../../../common/slices/api.slice'

// style
import {
  TransactionTypeText,
  TransactionText,
  Divider,
  SectionRow,
  SectionColumn,
  EditButton,
} from './style'
import { WarningBoxTitleRow } from '../shared-panel-styles'
import { Skeleton } from '../../shared/loading-skeleton/styles'
import { Column, Text } from '../../shared/style'

interface TransactionInfoProps {
  onToggleEditGas?: () => void
  transactionDetails: ParsedTransaction | undefined
  isZCashTransaction: boolean
  isBitcoinTransaction: boolean
  isERC721SafeTransferFrom: boolean
  isERC721TransferFrom: boolean
  transactionsNetwork: BraveWallet.NetworkInfo | undefined
  hasFeeEstimatesError: boolean
  isLoadingGasFee: boolean
  gasFee: string
  insufficientFundsError: boolean
  insufficientFundsForGasError: boolean

  // ERC20 Approve
  isERC20Approve?: boolean
  currentTokenAllowance?: string
  isCurrentAllowanceUnlimited?: boolean
}

export const TransactionInfo = ({
  onToggleEditGas,
  transactionDetails,
  isERC721SafeTransferFrom,
  isERC721TransferFrom,
  transactionsNetwork,
  hasFeeEstimatesError,
  isLoadingGasFee,
  gasFee,
  insufficientFundsError,
  insufficientFundsForGasError,
  isZCashTransaction,
  isBitcoinTransaction,
  isERC20Approve,
  isCurrentAllowanceUnlimited,
  currentTokenAllowance,
}: TransactionInfoProps) => {
  // queries
  const {
    isLoading: isLoadingDefaultFiatCurrency,
    data: defaultFiatCurrency = 'usd',
  } = useGetDefaultFiatCurrencyQuery()

  // exit early if no details
  if (!transactionDetails) {
    return null
  }

  const isLoadingGasFeeFiat = isLoadingDefaultFiatCurrency || isLoadingGasFee

  const { isSolanaTransaction, isFilecoinTransaction, isCardanoTransaction } =
    transactionDetails
  const feeLocale =
    isSolanaTransaction
    || isZCashTransaction
    || isBitcoinTransaction
    || isCardanoTransaction
      ? 'braveWalletConfirmTransactionTransactionFee'
      : 'braveWalletConfirmTransactionGasFee'

  const memoText = String.fromCharCode(...(transactionDetails.zcashMemo ?? []))

  // render
  return (
    <>
      {isFilecoinTransaction ? (
        <>
          {transactionDetails.gasPremium && (
            <SectionColumn>
              <Text
                textColor='secondary'
                variant='small.semibold'
              >
                {getLocale('braveWalletTransactionGasPremium')}
              </Text>
              <TransactionTypeText
                textColor='tertiary'
                variant='small.semibold'
              >
                {transactionsNetwork
                  && new Amount(transactionDetails.gasPremium)
                    .divideByDecimals(transactionsNetwork.decimals)
                    .formatAsAsset(6, transactionsNetwork.symbol)}
              </TransactionTypeText>
            </SectionColumn>
          )}

          {transactionDetails.gasLimit && (
            <SectionColumn>
              <Text
                textColor='secondary'
                variant='small.semibold'
              >
                {getLocale('braveWalletTransactionGasLimit')}
              </Text>
              <TransactionTypeText
                textColor='tertiary'
                variant='small.semibold'
              >
                {transactionsNetwork
                  && new Amount(transactionDetails.gasLimit)
                    .divideByDecimals(transactionsNetwork.decimals)
                    .formatAsAsset(6, transactionsNetwork.symbol)}
              </TransactionTypeText>
            </SectionColumn>
          )}

          {transactionDetails.gasFeeCap && (
            <SectionColumn>
              <Text
                textColor='secondary'
                variant='small.semibold'
              >
                {getLocale('braveWalletTransactionGasFeeCap')}
              </Text>
              <TransactionTypeText
                textColor='tertiary'
                variant='small.semibold'
              >
                {transactionsNetwork
                  && new Amount(transactionDetails.gasFeeCap)
                    .divideByDecimals(transactionsNetwork.decimals)
                    .formatAsAsset(6, transactionsNetwork.symbol)}
              </TransactionTypeText>
            </SectionColumn>
          )}
        </>
      ) : (
        <SectionRow>
          <Text
            textColor='secondary'
            variant='small.semibold'
          >
            {getLocale(
              isERC20Approve
                ? 'braveWalletAllowSpendTransactionFee'
                : feeLocale,
            )}
          </Text>

          {onToggleEditGas && (
            <EditButton onClick={onToggleEditGas}>
              {getLocale('braveWalletAllowSpendEditButton')}
            </EditButton>
          )}
        </SectionRow>
      )}

      {hasFeeEstimatesError ? (
        <TransactionText
          textColor='error'
          variant='small.regular'
        >
          {getLocale('braveWalletTransactionHasFeeEstimatesError')}
        </TransactionText>
      ) : isLoadingGasFee ? (
        <FeeSkeleton />
      ) : (
        <TransactionTypeText
          textColor='tertiary'
          variant='small.semibold'
        >
          {(transactionsNetwork
            && new Amount(gasFee)
              .divideByDecimals(transactionsNetwork.decimals)
              .formatAsAsset(6, transactionsNetwork.symbol))
            || ''}
        </TransactionTypeText>
      )}

      {hasFeeEstimatesError ? null : isLoadingGasFeeFiat ? (
        <FeeSkeleton />
      ) : (
        <TransactionText
          textColor='tertiary'
          variant='small.regular'
        >
          {new Amount(transactionDetails.gasFeeFiat).formatAsFiat(
            defaultFiatCurrency,
          )}
        </TransactionText>
      )}

      {/* TODO: ERC allowance checks */}

      {!isERC20Approve && (
        <>
          <Divider />

          <WarningBoxTitleRow>
            <Text
              textColor='secondary'
              variant='small.semibold'
            >
              {getLocale('braveWalletConfirmTransactionTotal')}{' '}
              {!isFilecoinTransaction && getLocale(feeLocale)}
            </Text>
          </WarningBoxTitleRow>
          <TransactionValue
            isErc721Transfer={isERC721SafeTransferFrom || isERC721TransferFrom}
            symbol={transactionDetails.symbol}
            valueExact={transactionDetails.valueExact}
          />

          {isFilecoinTransaction ? null : hasFeeEstimatesError ? (
            <TransactionText
              textColor='error'
              variant='small.regular'
            >
              {getLocale('braveWalletTransactionHasFeeEstimatesError')}
            </TransactionText>
          ) : isLoadingGasFee ? (
            <FeeSkeleton />
          ) : (
            <TransactionTypeText
              textColor='tertiary'
              variant='small.semibold'
            >
              +{' '}
              {transactionsNetwork
                && new Amount(gasFee)
                  .divideByDecimals(transactionsNetwork.decimals)
                  .formatAsAsset(6, transactionsNetwork.symbol)}
            </TransactionTypeText>
          )}

          {hasFeeEstimatesError ? null : isLoadingGasFeeFiat ? (
            <FeeSkeleton />
          ) : (
            <TransactionText
              textColor='tertiary'
              variant='small.regular'
            >
              {new Amount(transactionDetails.fiatTotal).formatAsFiat(
                defaultFiatCurrency,
              )}
            </TransactionText>
          )}
        </>
      )}

      {insufficientFundsForGasError && (
        <TransactionText
          textColor='error'
          variant='small.regular'
        >
          {getLocale('braveWalletSwapInsufficientFundsForGas')}
        </TransactionText>
      )}
      {!insufficientFundsForGasError && insufficientFundsError && (
        <TransactionText
          textColor='error'
          variant='small.regular'
        >
          {getLocale('braveWalletSwapInsufficientBalance')}
        </TransactionText>
      )}

      {isERC20Approve && (
        <>
          <Divider />

          <Text
            textColor='secondary'
            variant='small.semibold'
          >
            {getLocale('braveWalletAllowSpendCurrentAllowance')}
          </Text>
          <TransactionTypeText
            textColor='tertiary'
            variant='small.semibold'
          >
            {isCurrentAllowanceUnlimited
              ? getLocale('braveWalletTransactionApproveUnlimited')
              : currentTokenAllowance}{' '}
            {transactionDetails.symbol}
          </TransactionTypeText>

          <Divider />

          <Text
            textColor='secondary'
            variant='small.semibold'
          >
            {getLocale('braveWalletAllowSpendProposedAllowance')}
          </Text>
          <TransactionTypeText
            textColor='tertiary'
            variant='small.semibold'
          >
            {transactionDetails.isApprovalUnlimited
              ? getLocale('braveWalletTransactionApproveUnlimited')
              : new Amount(transactionDetails.valueExact).formatAsAsset(
                  undefined,
                  transactionDetails.symbol,
                )}
          </TransactionTypeText>
        </>
      )}

      {memoText && (
        <SectionColumn>
          <Divider />
          <Text
            textColor='secondary'
            variant='small.semibold'
          >
            {getLocale('braveWalletMemo')}
          </Text>
          <TransactionTypeText
            textColor='tertiary'
            variant='small.semibold'
          >
            {memoText}
          </TransactionTypeText>
        </SectionColumn>
      )}
    </>
  )
}

const FeeSkeleton = () => {
  return (
    <Column
      fullHeight
      fullWidth
      alignItems={'flex-start'}
      justifyContent='flex-start'
    >
      <Skeleton
        width={'40px'}
        height={'12px'}
        enableAnimation
      />
    </Column>
  )
}

const TransactionValue = ({
  isErc721Transfer,
  symbol,
  valueExact,
}: {
  isErc721Transfer: boolean
  valueExact: string
  symbol: string
}) => {
  /**
   * This will need updating if we ever switch to using per-locale formatting,
   * since `.` isn't always the decimal separator
   */
  const transactionValueParts = (
    isErc721Transfer
      ? valueExact
      : new Amount(valueExact).format(undefined, true)
  ).split('.')

  /**
   * Inserts a <wbr /> tag between the integer and decimal portions of the value
   * for wrapping
   * This will need updating if we ever switch to using per-locale formatting
   */
  const transactionValueText = (
    <span>
      {transactionValueParts.map((part, i, { length }) => [
        part,
        ...(i < length - 1 ? ['.'] : []),
        <wbr key={part} />,
      ])}
    </span>
  )

  // render
  return (
    <TransactionTypeText
      textColor='tertiary'
      variant='small.semibold'
    >
      {transactionValueText} {symbol}
    </TransactionTypeText>
  )
}
