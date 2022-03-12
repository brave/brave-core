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
import { formatDateAsRelative } from '../../../utils/datetime-utils'
import { mojoTimeDeltaToJSDate } from '../../../../common/mojomUtils'

// Hooks
import { useTransactionParser } from '../../../common/hooks'
import { SwapExchangeProxy } from '../../../common/hooks/address-labels'

// Styled Components
import {
  DetailTextDarkBold,
  DetailTextDark
} from '../shared-panel-styles'

import { StatusBubble } from '../../shared/style'

import {
  DetailColumn,
  FromCircle,
  StatusAndTimeRow,
  StatusRow,
  StyledWrapper,
  ToCircle,
  TransactionDetailRow
} from './style'
import { reduceAddress } from '../../../utils/reduce-address'
import { TransactionIntentDescription } from './transaction-intent-description'

export interface Props {
  selectedNetwork: BraveWallet.NetworkInfo
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

  const transactionIntentLocale = React.useMemo((): React.ReactNode => {
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
        return (<DetailTextDark>
          {`${transactionDetails.senderLabel} ${text} ${transactionDetails.fiatValue
            .formatAsFiat(defaultCurrencies.fiat)} (${transactionDetails.formattedNativeCurrencyTotal}${erc721ID})`}
        </DetailTextDark>)
      }
    }
  }, [transaction])

  const transactionIntentDescription = React.useMemo(() => {
    switch (true) {
      case transaction.txType === BraveWallet.TransactionType.ERC20Approve: {
        return (
          <TransactionIntentDescription
            from={`${transactionDetails.value} ${transactionDetails.symbol}`}
            to={transactionDetails.approvalTargetLabel || ''}
          />
        )
      }

      // FIXME: Add as new TransactionType on the service side.
      case transaction.txDataUnion.ethTxData1559?.baseData.to.toLowerCase() === SwapExchangeProxy: {
        return (
          <TransactionIntentDescription
            from={`${transactionDetails.value} ${transactionDetails.symbol}`}
            to={transactionDetails.recipientLabel}
          />
        )
      }

      case transaction.txType === BraveWallet.TransactionType.ETHSend:
      case transaction.txType === BraveWallet.TransactionType.ERC20Transfer:
      case transaction.txType === BraveWallet.TransactionType.ERC721TransferFrom:
      case transaction.txType === BraveWallet.TransactionType.ERC721SafeTransferFrom:
      default: {
        return (
          <TransactionIntentDescription
            from={`${reduceAddress(transactionDetails.sender)} `}
            to={reduceAddress(transactionDetails.recipient)}
          />
        )
      }
    }
  }, [transactionDetails])

  return (
    <StyledWrapper onClick={onClickTransaction}>
      <DetailColumn>
        <TransactionDetailRow>
          <DetailColumn>
            <FromCircle orb={fromOrb} />
            <ToCircle orb={toOrb} />
          </DetailColumn>

          <DetailColumn>
            <span>
              <DetailTextDark>{transactionIntentLocale}</DetailTextDark>&nbsp;
              {transactionIntentDescription}
            </span>
            <StatusAndTimeRow>
              <DetailTextDarkBold>
                {formatDateAsRelative(mojoTimeDeltaToJSDate(transactionDetails.createdTime))}
              </DetailTextDarkBold>

              <StatusRow>
                <StatusBubble status={transactionDetails.status} />
                <DetailTextDarkBold>
                  {getTransactionStatusString(transactionDetails.status)}
                </DetailTextDarkBold>
              </StatusRow>
            </StatusAndTimeRow>
          </DetailColumn>

        </TransactionDetailRow>
      </DetailColumn>

    </StyledWrapper>
  )
}

export default TransactionsListItem
