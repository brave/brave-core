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
import Amount from '../../../utils/amount'

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

const { ERC20Approve, ERC721TransferFrom, ERC721SafeTransferFrom } = BraveWallet.TransactionType

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
    // approval
    if (transaction.txType === ERC20Approve) {
      return toProperCase(getLocale('braveWalletApprovalTransactionIntent'))
    }

    // Detect sending to 0x Exchange Proxy
    if (transactionDetails.isSwap) {
      return getLocale('braveWalletSwap')
    }

    // default or when: [ETHSend, ERC20Transfer, ERC721TransferFrom, ERC721SafeTransferFrom].includes(transaction.txType)
    let erc721ID = transaction.txType === ERC721TransferFrom || transaction.txType === ERC721SafeTransferFrom
      ? ' ' + transactionDetails.erc721TokenId
      : ''
    return (
      <DetailTextDark>
        {`${
            toProperCase(getLocale('braveWalletTransactionSent'))} ${
            transactionDetails.fiatValue.formatAsFiat(defaultCurrencies.fiat) || '...'
          } (${
            transactionDetails.formattedNativeCurrencyTotal || '...'
          }${
            erc721ID
          })`}
      </DetailTextDark>
    )
  }, [transaction])

  const transactionIntentDescription = React.useMemo(() => {
    // default or when: [ETHSend, ERC20Transfer, ERC721TransferFrom, ERC721SafeTransferFrom].includes(transaction.txType)
    let from = `${reduceAddress(transactionDetails.sender)} `
    let to = reduceAddress(transactionDetails.recipient)
    const wrapFromText = transaction.txType === ERC20Approve || transaction.txDataUnion.ethTxData1559?.baseData.to.toLowerCase() === SwapExchangeProxy

    if (transaction.txType === ERC20Approve) {
      // Approval
      from = new Amount(transactionDetails.value).formatAsAsset(undefined, transactionDetails.symbol)
      to = transactionDetails.approvalTargetLabel || ''
    } else if (transaction.txDataUnion.ethTxData1559?.baseData.to.toLowerCase() === SwapExchangeProxy) {
      // Brave Swap
      // FIXME: Add as new TransactionType on the service side.
      from = `${transactionDetails.value} ${transactionDetails.symbol}`
      to = transactionDetails.recipientLabel
    }

    return <TransactionIntentDescription from={from} to={to} wrapFrom={wrapFromText} />
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
