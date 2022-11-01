// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'
import { WalletState } from '../../../constants/types'
import Amount from '../../../utils/amount'
import { getLocale } from '../../../../common/locale'
import { usePendingTransactions } from '../../../common/hooks/use-pending-transaction'
import {
  TransactionTitle,
  TransactionTypeText,
  TransactionText, Divider,
  SectionRow,
  SectionColumn,
  EditButton
} from './style'
import { WarningBoxTitleRow } from '../shared-panel-styles'

interface TransactionInfoProps {
  onToggleEditGas?: () => void
}
export const TransactionInfo = ({
  onToggleEditGas
}: TransactionInfoProps) => {
  const {
    transactionDetails,
    isERC721SafeTransferFrom,
    isERC721TransferFrom,
    isSolanaTransaction,
    isFilecoinTransaction,
    transactionsNetwork,
    sendOptions
  } = usePendingTransactions()

  // redux
  const {
    defaultCurrencies
  } = useSelector((state: { wallet: WalletState }) => state.wallet)

  // exit early if no details
  if (!transactionDetails) {
    return null
  }

  /**
   * This will need updating if we ever switch to using per-locale formatting,
   * since `.` isnt always the decimal seperator
  */
  const transactionValueParts = (
    (!isERC721SafeTransferFrom && !isERC721TransferFrom)
      ? new Amount(transactionDetails.valueExact)
        .format(undefined, true)
      : transactionDetails.valueExact
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
  return <>
    {!isFilecoinTransaction &&
      <SectionRow>
        <TransactionTitle>
          {
            isSolanaTransaction
              ? getLocale('braveWalletConfirmTransactionTransactionFee')
              : getLocale('braveWalletConfirmTransactionGasFee')
          }
        </TransactionTitle>

        {!isSolanaTransaction && onToggleEditGas &&
          <EditButton onClick={onToggleEditGas}>
            {getLocale('braveWalletAllowSpendEditButton')}
          </EditButton>
        }
      </SectionRow>
    }

    {isFilecoinTransaction &&
      <>
        {transactionDetails.gasPremium &&
          <SectionColumn>
            <TransactionTitle>Gas Premium</TransactionTitle>
            <TransactionTypeText>
              {transactionsNetwork && new Amount(transactionDetails.gasPremium)
                .divideByDecimals(transactionsNetwork.decimals)
                .formatAsAsset(6, transactionsNetwork.symbol)}
            </TransactionTypeText>
          </SectionColumn>
        }

        {transactionDetails.gasLimit &&
          <SectionColumn>
            <TransactionTitle>Gas Limit</TransactionTitle>
            <TransactionTypeText>
              {transactionsNetwork && new Amount(transactionDetails.gasLimit)
                .divideByDecimals(transactionsNetwork.decimals)
                .formatAsAsset(6, transactionsNetwork.symbol)}
            </TransactionTypeText>
          </SectionColumn>
        }

        {transactionDetails.gasFeeCap &&
          <SectionColumn>
            <TransactionTitle>Gas Fee Cap</TransactionTitle>
            <TransactionTypeText>
              {transactionsNetwork && new Amount(transactionDetails.gasFeeCap)
                .divideByDecimals(transactionsNetwork.decimals)
                .formatAsAsset(6, transactionsNetwork.symbol)}
            </TransactionTypeText>
          </SectionColumn>
        }
      </>
    }

    <TransactionTypeText>
      {transactionsNetwork && new Amount(transactionDetails.gasFee)
        .divideByDecimals(transactionsNetwork.decimals)
        .formatAsAsset(6, transactionsNetwork.symbol)}
    </TransactionTypeText>

    <TransactionText>
      {new Amount(transactionDetails.gasFeeFiat)
        .formatAsFiat(defaultCurrencies.fiat)}
    </TransactionText>

    <Divider />

    <WarningBoxTitleRow>
      <TransactionTitle>
        {getLocale('braveWalletConfirmTransactionTotal')}
        {' '}
        {!isFilecoinTransaction &&
          <>
            ({
              isSolanaTransaction
                ? getLocale('braveWalletConfirmTransactionAmountFee')
                : getLocale('braveWalletConfirmTransactionAmountGas')
            })
          </>
        }
      </TransactionTitle>
    </WarningBoxTitleRow>

    <TransactionTypeText>
      {transactionValueText} {transactionDetails.symbol}
    </TransactionTypeText>
    {!isFilecoinTransaction &&
      <TransactionTypeText>
        + {transactionsNetwork && new Amount(transactionDetails.gasFee)
          .divideByDecimals(transactionsNetwork.decimals)
          .formatAsAsset(6, transactionsNetwork.symbol)}
      </TransactionTypeText>
    }

    <TransactionText hasError={false}>
      {transactionDetails.fiatTotal.formatAsFiat(defaultCurrencies.fiat)}
    </TransactionText>

    {transactionDetails.insufficientFundsForGasError &&
      <TransactionText hasError={true}>
        {getLocale('braveWalletSwapInsufficientFundsForGas')}
      </TransactionText>
    }

    {transactionDetails.insufficientFundsForGasError === false &&
      transactionDetails.insufficientFundsError &&
      <TransactionText hasError={true}>
        {getLocale('braveWalletSwapInsufficientBalance')}
      </TransactionText>
    }

    {sendOptions &&
      <Divider />
    }
    {sendOptions?.maxRetries &&
      <>
        <TransactionTitle>{getLocale('braveWalletSolanaMaxRetries')}</TransactionTitle>
        <TransactionTypeText>{Number(sendOptions.maxRetries.maxRetries)}</TransactionTypeText>
      </>
    }
    {sendOptions?.preflightCommitment &&
      <>
        <TransactionTitle>{getLocale('braveWalletSolanaPreflightCommitment')}</TransactionTitle>
        <TransactionTypeText>{sendOptions.preflightCommitment}</TransactionTypeText>
      </>
    }
    {sendOptions?.skipPreflight &&
      <>
        <TransactionTitle>{getLocale('braveWalletSolanaSkipPreflight')}</TransactionTitle>
        <TransactionTypeText>{sendOptions.skipPreflight.skipPreflight.toString()}</TransactionTypeText>
      </>
    }
  </>
}
