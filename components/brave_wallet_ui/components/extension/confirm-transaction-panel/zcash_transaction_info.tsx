// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import Amount from '../../../utils/amount'
import { getLocale } from '../../../../common/locale'

// hooks
import { usePendingTransactions } from '../../../common/hooks/use-pending-transaction'
import {
  useGetDefaultFiatCurrencyQuery //
} from '../../../common/slices/api.slice'

// style
import {
  TransactionTitle,
  TransactionTypeText,
  TransactionText, Divider,
  SectionRow,
  EditButton
} from './style'
import { WarningBoxTitleRow } from '../shared-panel-styles'
import { Skeleton } from '../../shared/loading-skeleton/styles'
import { Column } from '../../shared/style'

interface TransactionInfoProps {
  onToggleEditGas?: () => void
}
export const ZCashTransactionInfo = ({
  onToggleEditGas
}: TransactionInfoProps) => {
  const {
    transactionDetails,
    transactionsNetwork,
    hasFeeEstimatesError,
    isLoadingGasFee,
    gasFee,
    insufficientFundsError,
    insufficientFundsForGasError
  } = usePendingTransactions()

  // queries
  const {
    isLoading: isLoadingDefaultFiatCurrency,
    data: defaultFiatCurrency = 'usd'
  } = useGetDefaultFiatCurrencyQuery()

  // exit early if no details
  if (!transactionDetails) {
    return null
  }

  const isLoadingGasFeeFiat = isLoadingDefaultFiatCurrency || isLoadingGasFee

  /**
   * This will need updating if we ever switch to using per-locale formatting,
   * since `.` isn't always the decimal separator
  */
  const transactionValueParts = (
    (new Amount(transactionDetails.valueExact).format(undefined, true))
  ).split('.')

  /**
   * Inserts a <wbr /> tag between the integer and decimal portions of the value for wrapping
   * This will need updating if we ever switch to using per-locale formatting
   */
  const transactionValueText = <span>
    {transactionValueParts.map((part, i, { length }) => [
      part,
      ...(i < (length - 1) ? ['.'] : []),
      <wbr key={part} />
    ])}
  </span>

  // render
  return (
    <>
      {(
        <SectionRow>
          <TransactionTitle>
            {getLocale('braveWalletConfirmTransactionTransactionFee')}
          </TransactionTitle>

          { onToggleEditGas && (
            <EditButton onClick={onToggleEditGas}>
              {getLocale('braveWalletAllowSpendEditButton')}
            </EditButton>
          )}
        </SectionRow>
      )}

      {hasFeeEstimatesError ? (
        <TransactionText hasError={true}>
          {getLocale('braveWalletTransactionHasFeeEstimatesError')}
        </TransactionText>
      ) : isLoadingGasFee ? (
        <Column
          fullHeight
          fullWidth
          alignItems={'flex-start'}
          justifyContent='flex-start'
        >
          <Skeleton width={'40px'} height={'12px'} enableAnimation />
        </Column>
      ) : (
        <TransactionTypeText>
          {(transactionsNetwork &&
            new Amount(gasFee)
              .divideByDecimals(transactionsNetwork.decimals)
              .formatAsAsset(6, transactionsNetwork.symbol)) ||
            ''}
        </TransactionTypeText>
      )}

      {hasFeeEstimatesError ? null : isLoadingGasFeeFiat ? (
        <Column
          fullHeight
          fullWidth
          alignItems={'flex-start'}
          justifyContent='flex-start'
        >
          <Skeleton width={'40px'} height={'12px'} enableAnimation />
        </Column>
      ) : (
        <TransactionText>
          {new Amount(transactionDetails.gasFeeFiat).formatAsFiat(
            defaultFiatCurrency
          )}
        </TransactionText>
      )}
      <Divider />
      <WarningBoxTitleRow>
        <TransactionTitle>
          {getLocale('braveWalletConfirmTransactionTotal')}{' '}
          {getLocale('braveWalletConfirmTransactionAmountFee')}
        </TransactionTitle>
      </WarningBoxTitleRow>
      <TransactionTypeText>
        {transactionValueText} {transactionDetails.symbol}
      </TransactionTypeText>
      {(hasFeeEstimatesError ? (
          <TransactionText hasError={true}>
            {getLocale('braveWalletTransactionHasFeeEstimatesError')}
          </TransactionText>
        ) : isLoadingGasFee ? (
          <Column
            fullHeight
            fullWidth
            alignItems={'flex-start'}
            justifyContent='flex-start'
          >
            <Skeleton width={'40px'} height={'12px'} enableAnimation />
          </Column>
        ) : (
          <TransactionTypeText>
            +{' '}
            {transactionsNetwork &&
              new Amount(gasFee)
                .divideByDecimals(transactionsNetwork.decimals)
                .formatAsAsset(6, transactionsNetwork.symbol)}
          </TransactionTypeText>
        ))}

      {hasFeeEstimatesError ? null : isLoadingGasFeeFiat ? (
        <Column
          fullHeight
          fullWidth
          alignItems={'flex-start'}
          justifyContent='flex-start'
        >
          <Skeleton width={'40px'} height={'12px'} enableAnimation />
        </Column>
      ) : (
        <TransactionText hasError={false}>
          {new Amount(transactionDetails.fiatTotal).formatAsFiat(
            defaultFiatCurrency
          )}
        </TransactionText>
      )}

      {insufficientFundsForGasError && (
        <TransactionText hasError={true}>
          {getLocale('braveWalletSwapInsufficientFundsForGas')}
        </TransactionText>
      )}
      {!insufficientFundsForGasError && insufficientFundsError && (
        <TransactionText hasError={true}>
          {getLocale('braveWalletSwapInsufficientBalance')}
        </TransactionText>
      )}
    </>
  )
}
