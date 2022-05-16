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
  EditButton
} from './style'
import { WarningBoxTitleRow } from '../shared-panel-styles'

interface TransactionInfoProps {
  onToggleEditGas: () => void
}
export const TransactionInfo = ({
  onToggleEditGas
}: TransactionInfoProps) => {
  const {
    transactionDetails,
    isERC721SafeTransferFrom,
    isERC721TransferFrom,
    isSolanaTransaction,
    transactionsNetwork
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
    <SectionRow>
      <TransactionTitle>
        {
          isSolanaTransaction
            ? getLocale('braveWalletConfirmTransactionTransactionFee')
            : getLocale('braveWalletConfirmTransactionGasFee')
        }
      </TransactionTitle>

      {!isSolanaTransaction &&
        <EditButton onClick={onToggleEditGas}>
          {getLocale('braveWalletAllowSpendEditButton')}
        </EditButton>
      }
    </SectionRow>

    <TransactionTypeText>
      {new Amount(transactionDetails.gasFee)
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
        ({
          isSolanaTransaction
            ? getLocale('braveWalletConfirmTransactionAmountFee')
            : getLocale('braveWalletConfirmTransactionAmountGas')
        })
      </TransactionTitle>
    </WarningBoxTitleRow>

    <TransactionTypeText>
      {transactionValueText} {transactionDetails.symbol} +
    </TransactionTypeText>
    <TransactionTypeText>
      {new Amount(transactionDetails.gasFee)
        .divideByDecimals(transactionsNetwork.decimals)
        .formatAsAsset(6, transactionsNetwork.symbol)}
    </TransactionTypeText>

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
  </>
}
