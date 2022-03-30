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
  EditButton, WarningTitleRow
} from './style'

interface TransactionInfoProps {
  onToggleEditGas: () => void
  isSolanaSystemTransfer?: boolean
}
export const TransactionInfo = ({
  onToggleEditGas,
  isSolanaSystemTransfer
}: TransactionInfoProps) => {
  const {
    transactionDetails, isERC721SafeTransferFrom, isERC721TransferFrom
  } = usePendingTransactions()

  // redux
  const {
    selectedNetwork, defaultCurrencies
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
      <wbr />
    ])}
  </span>

  // render
  return <>
    <SectionRow>
      <TransactionTitle>
        {
          isSolanaSystemTransfer
            ? getLocale('braveWalletConfirmTransactionTransactionFee')
            : getLocale('braveWalletConfirmTransactionGasFee')
        }
      </TransactionTitle>

      {!isSolanaSystemTransfer &&
        <EditButton onClick={onToggleEditGas}>
          {getLocale('braveWalletAllowSpendEditButton')}
        </EditButton>
      }
    </SectionRow>

    <TransactionTypeText>
      {new Amount(transactionDetails.gasFee)
        .divideByDecimals(selectedNetwork.decimals)
        .formatAsAsset(6, selectedNetwork.symbol)}
    </TransactionTypeText>

    <TransactionText>
      {new Amount(transactionDetails.gasFeeFiat)
        .formatAsFiat(defaultCurrencies.fiat)}
    </TransactionText>

    <Divider />

    <WarningTitleRow>
      <TransactionTitle>
        {getLocale('braveWalletConfirmTransactionTotal')}
        {' '}
        ({
          isSolanaSystemTransfer
            ? getLocale('braveWalletConfirmTransactionAmountFee')
            : getLocale('braveWalletConfirmTransactionAmountGas')
        })
      </TransactionTitle>
    </WarningTitleRow>

    <TransactionTypeText>
      {transactionValueText} {transactionDetails.symbol} +
    </TransactionTypeText>
    <TransactionTypeText>
      {new Amount(transactionDetails.gasFee)
        .divideByDecimals(selectedNetwork.decimals)
        .formatAsAsset(6, selectedNetwork.symbol)}
    </TransactionTypeText>

    <TransactionText
      hasError={transactionDetails.insufficientFundsError}
    >
      {transactionDetails.insufficientFundsError
        ? `${getLocale('braveWalletSwapInsufficientBalance')} `
        : ''}
      {transactionDetails.fiatTotal
        .formatAsFiat(defaultCurrencies.fiat)}
    </TransactionText>
  </>
}
