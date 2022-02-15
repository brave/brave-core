import * as React from 'react'
import * as EthereumBlockies from 'ethereum-blockies'

import { getLocale } from '../../../../common/locale'
import {
  BraveWallet,
  WalletAccountType,
  DefaultCurrencies
} from '../../../constants/types'

// Utils
import { toProperCase } from '../../../utils/string-utils'
import { getTransactionStatusString } from '../../../utils/tx-utils'
import { mojoTimeDeltaToJSDate, formatDateAsRelative } from '../../../utils/datetime-utils'
import {
  formatFiatAmountWithCommasAndDecimals,
  formatTokenAmountWithCommasAndDecimals
} from '../../../utils/format-prices'

// Hooks
import { useTransactionParser } from '../../../common/hooks'
import { SwapExchangeProxy } from '../../../common/hooks/address-labels'

// Styled Components
import {
  DetailTextDarkBold,
  DetailTextDark,
  DetailTextLight
} from '../shared-panel-styles'

import { StatusBubble } from '../../shared/style'

import {
  ArrowIcon,
  BalanceColumn,
  DetailColumn,
  DetailRow,
  FromCircle,
  StatusRow,
  StyledWrapper,
  ToCircle,
  TransactionDetailRow
} from './style'

export interface Props {
  selectedNetwork: BraveWallet.EthereumChain
  transaction: BraveWallet.TransactionInfo
  account?: WalletAccountType
  accounts: WalletAccountType[]
  visibleTokens: BraveWallet.BlockchainToken[]
  transactionSpotPrices: BraveWallet.AssetPrice[]
  defaultCurrencies: DefaultCurrencies
  onSelectTransaction: (transaction: BraveWallet.TransactionInfo) => void
}

const TransactionsListItem = (props: Props) => {
  const {
    transaction,
    selectedNetwork,
    visibleTokens,
    transactionSpotPrices,
    accounts,
    defaultCurrencies,
    onSelectTransaction
  } = props

  const parseTransaction = useTransactionParser(selectedNetwork, accounts, transactionSpotPrices, visibleTokens)
  const transactionDetails = React.useMemo(
    () => parseTransaction(transaction),
    [transaction]
  )

  const fromOrb = React.useMemo(() => {
    return EthereumBlockies.create({ seed: transactionDetails.sender.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [transactionDetails.sender])

  const toOrb = React.useMemo(() => {
    return EthereumBlockies.create({ seed: transactionDetails.recipient.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [transactionDetails.recipient])

  const onClickTransaction = () => {
    onSelectTransaction(transaction)
  }

  const transactionIntentLocale = React.useMemo((): string => {
    switch (true) {
      case transaction.txType === BraveWallet.TransactionType.ERC20Approve: {
        const text = getLocale('braveWalletApprovalTransactionIntent')
        return `${toProperCase(text)} ${transactionDetails.symbol}`
      }

      // Detect sending to 0x Exchange Proxy
      case transactionDetails.isSwap: {
        return getLocale('braveWalletSwap')
      }

      case transaction.txType === BraveWallet.TransactionType.ETHSend:
      case transaction.txType === BraveWallet.TransactionType.ERC20Transfer:
      case transaction.txType === BraveWallet.TransactionType.ERC721TransferFrom:
      case transaction.txType === BraveWallet.TransactionType.ERC721SafeTransferFrom:
      default: {
        const text = getLocale('braveWalletTransactionSent')
        const erc721ID = transaction.txType === BraveWallet.TransactionType.ERC721TransferFrom ||
          transaction.txType === BraveWallet.TransactionType.ERC721SafeTransferFrom
          ? ' ' + transactionDetails.erc721TokenId
          : ''
        return `${toProperCase(text)} ${transactionDetails.symbol}${erc721ID}`
      }
    }
  }, [transaction])

  const transactionIntentDescription = React.useMemo(() => {
    switch (true) {
      case transaction.txType === BraveWallet.TransactionType.ERC20Approve: {
        return (
          <>
            <DetailRow>
              <DetailTextLight>
                {transactionDetails.value}{' '}
                {transactionDetails.symbol}
              </DetailTextLight>
            </DetailRow>
            <DetailRow>
              <ArrowIcon />
              <DetailTextLight>
                {transactionDetails.approvalTargetLabel}
              </DetailTextLight>
            </DetailRow>

          </>
        )
      }

      // FIXME: Add as new TransactionType on the service side.
      case transaction.txDataUnion.ethTxData1559?.baseData.to.toLowerCase() === SwapExchangeProxy: {
        return (
          <>
            <DetailRow>
              <DetailTextLight>
                {transactionDetails.value}{' '}
                {transactionDetails.symbol}
              </DetailTextLight>
            </DetailRow>
            <DetailRow>
              <ArrowIcon />
              <DetailTextLight>
                {transactionDetails.recipientLabel}
              </DetailTextLight>
            </DetailRow>
          </>
        )
      }

      case transaction.txType === BraveWallet.TransactionType.ETHSend:
      case transaction.txType === BraveWallet.TransactionType.ERC20Transfer:
      case transaction.txType === BraveWallet.TransactionType.ERC721TransferFrom:
      case transaction.txType === BraveWallet.TransactionType.ERC721SafeTransferFrom:
      default: {
        return (
          <>
            <DetailRow>
              <DetailTextLight>
                {transactionDetails.senderLabel}{' '}
              </DetailTextLight>
            </DetailRow>
            <DetailRow>
              <ArrowIcon />
              <DetailTextLight>
                {transactionDetails.recipientLabel}
              </DetailTextLight>
            </DetailRow>
          </>
        )
      }
    }
  }, [transactionDetails])

  return (
    <StyledWrapper onClick={onClickTransaction}>
      <TransactionDetailRow>
        <FromCircle orb={fromOrb} />
        <ToCircle orb={toOrb} />
        <DetailColumn>
          <DetailRow>
            <DetailTextDark>
              {transactionIntentLocale}
            </DetailTextDark>&nbsp;
            <DetailTextLight>-</DetailTextLight>&nbsp;
            <DetailTextDarkBold>{formatDateAsRelative(mojoTimeDeltaToJSDate(transactionDetails.createdTime))}</DetailTextDarkBold>
          </DetailRow>
          {transactionIntentDescription}
        </DetailColumn>
      </TransactionDetailRow>
      <DetailRow>
        <BalanceColumn>
          <DetailTextDark>{formatFiatAmountWithCommasAndDecimals(transactionDetails.fiatValue, defaultCurrencies.fiat)}</DetailTextDark>
          <DetailTextLight>{formatTokenAmountWithCommasAndDecimals(transactionDetails.nativeCurrencyTotal, selectedNetwork.symbol)}</DetailTextLight>
          <StatusRow>
            <StatusBubble status={transactionDetails.status} />
            <DetailTextDarkBold>
              {getTransactionStatusString(transactionDetails.status)}
            </DetailTextDarkBold>
          </StatusRow>
        </BalanceColumn>
      </DetailRow>
    </StyledWrapper>
  )
}

export default TransactionsListItem
