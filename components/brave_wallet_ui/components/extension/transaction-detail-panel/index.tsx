import * as React from 'react'
import * as EthereumBlockies from 'ethereum-blockies'

// Hooks
import { useExplorer, useTransactionParser } from '../../../common/hooks'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import { getTransactionStatusString } from '../../../utils/tx-utils'
import { toProperCase } from '../../../utils/string-utils'
import { mojoTimeDeltaToJSDate } from '../../../utils/datetime-utils'
import {
  formatFiatAmountWithCommasAndDecimals,
  formatTokenAmountWithCommasAndDecimals
} from '../../../utils/format-prices'

import { getLocale } from '../../../../common/locale'
import {
  BraveWallet,
  WalletAccountType,
  DefaultCurrencies
} from '../../../constants/types'
import Header from '../../buy-send-swap/select-header'

// Styled Components
import {
  StyledWrapper,
  OrbContainer,
  FromCircle,
  ToCircle,
  DetailRow,
  DetailTitle,
  DetailButton,
  StatusRow,
  BalanceColumn,
  TransactionValue,
  PanelDescription,
  SpacerText,
  FromToRow,
  AccountNameText,
  ArrowIcon
} from './style'

import {
  DetailTextDarkBold,
  DetailTextDark
} from '../shared-panel-styles'

import { StatusBubble } from '../../shared/style'

export interface Props {
  transaction: BraveWallet.TransactionInfo
  selectedNetwork: BraveWallet.EthereumChain
  accounts: WalletAccountType[]
  visibleTokens: BraveWallet.BlockchainToken[]
  transactionSpotPrices: BraveWallet.AssetPrice[]
  defaultCurrencies: DefaultCurrencies
  onBack: () => void
  onRetryTransaction: (transaction: BraveWallet.TransactionInfo) => void
  onSpeedupTransaction: (transaction: BraveWallet.TransactionInfo) => void
  onCancelTransaction: (transaction: BraveWallet.TransactionInfo) => void
}

const TransactionDetailPanel = (props: Props) => {
  const {
    transaction,
    selectedNetwork,
    accounts,
    visibleTokens,
    transactionSpotPrices,
    defaultCurrencies,
    onBack,
    onRetryTransaction,
    onSpeedupTransaction,
    onCancelTransaction
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

  const onClickViewOnBlockExplorer = useExplorer(selectedNetwork)

  const onClickRetryTransaction = () => {
    if (transaction) {
      onRetryTransaction(transaction)
    }
  }

  const onClickSpeedupTransaction = () => {
    if (transaction) {
      onSpeedupTransaction(transaction)
    }
  }

  const onClickCancelTransaction = () => {
    if (transaction) {
      onCancelTransaction(transaction)
    }
  }

  const transactionTitle = React.useMemo((): string => {
    if (transactionDetails.isSwap) {
      return toProperCase(getLocale('braveWalletSwap'))
    }
    if (transaction.txType === BraveWallet.TransactionType.ERC20Approve) {
      return toProperCase(getLocale('braveWalletApprovalTransactionIntent'))
    }
    return toProperCase(getLocale('braveWalletTransactionSent'))
  }, [transactionDetails, transaction])

  const transactionValue = React.useMemo((): string => {
    if (transaction.txType === BraveWallet.TransactionType.ERC721TransferFrom ||
      transaction.txType === BraveWallet.TransactionType.ERC721SafeTransferFrom) {
      return transactionDetails.erc721BlockchainToken?.name + ' ' + transactionDetails.erc721TokenId
    }
    return formatTokenAmountWithCommasAndDecimals(transactionDetails.value, transactionDetails.symbol)
  }, [transactionDetails, transaction])

  const transactionFiatValue = React.useMemo((): string => {
    if (transaction.txType !== BraveWallet.TransactionType.ERC721TransferFrom &&
      transaction.txType !== BraveWallet.TransactionType.ERC721SafeTransferFrom &&
      transaction.txType !== BraveWallet.TransactionType.ERC20Approve) {
      return formatFiatAmountWithCommasAndDecimals(transactionDetails.fiatValue, defaultCurrencies.fiat)
    }
    return ''
  }, [transactionDetails, transaction, defaultCurrencies])

  return (
    <StyledWrapper>
      <Header
        title={getLocale('braveWalletTransactionDetails')}
        onBack={onBack}
      />
      <OrbContainer>
        <FromCircle orb={fromOrb} />
        <ToCircle orb={toOrb} />
      </OrbContainer>
      <FromToRow>
        <AccountNameText>{transactionDetails.senderLabel}</AccountNameText>
        <ArrowIcon />
        <AccountNameText>{transactionDetails.recipientLabel}</AccountNameText>
      </FromToRow>
      <PanelDescription>{transactionTitle}</PanelDescription>
      <TransactionValue>{transactionValue}</TransactionValue>
      <PanelDescription>{transactionFiatValue}</PanelDescription>
      <DetailRow>
        <DetailTitle>
          {getLocale('braveWalletAllowSpendTransactionFee')}
        </DetailTitle>
        <BalanceColumn>
          <DetailTextDark>{transactionDetails.gasFee} {selectedNetwork.symbol}</DetailTextDark>
          <DetailTextDark>{formatFiatAmountWithCommasAndDecimals(transactionDetails.gasFeeFiat, defaultCurrencies.fiat)}</DetailTextDark>
        </BalanceColumn>
      </DetailRow>
      <DetailRow>
        <DetailTitle>
          {getLocale('braveWalletTransactionDetailDate')}
        </DetailTitle>
        <DetailTextDark>
          {mojoTimeDeltaToJSDate(transactionDetails.createdTime).toUTCString()}
        </DetailTextDark>
      </DetailRow>
      {transactionDetails.status !== BraveWallet.TransactionStatus.Rejected &&
        <DetailRow>
          <DetailTitle>
            {getLocale('braveWalletTransactionDetailHash')}
          </DetailTitle>
          <DetailButton onClick={onClickViewOnBlockExplorer('tx', transaction?.txHash)}>
            {reduceAddress(transaction.txHash)}
          </DetailButton>
        </DetailRow>
      }
      <DetailRow>
        <DetailTitle>
          {getLocale('braveWalletTransactionDetailNetwork')}
        </DetailTitle>
        <DetailTextDark>
          {selectedNetwork.chainName}
        </DetailTextDark>
      </DetailRow>
      <DetailRow>
        <DetailTitle>
          {getLocale('braveWalletTransactionDetailStatus')}
        </DetailTitle>
        <StatusRow>
          <StatusBubble status={transactionDetails.status} />
          <DetailTextDarkBold>
            {getTransactionStatusString(transactionDetails.status)}
          </DetailTextDarkBold>
        </StatusRow>
      </DetailRow>
      {(transactionDetails.status === BraveWallet.TransactionStatus.Approved || transactionDetails.status === BraveWallet.TransactionStatus.Submitted) &&
        <DetailRow>
          <DetailTitle />
          <StatusRow>
            <DetailButton onClick={onClickSpeedupTransaction}>{getLocale('braveWalletTransactionDetailSpeedUp')}</DetailButton>
            <SpacerText>|</SpacerText>
            <DetailButton onClick={onClickCancelTransaction}>{getLocale('braveWalletBackupButtonCancel')}</DetailButton>
          </StatusRow>
        </DetailRow>
      }
      {transactionDetails.status === BraveWallet.TransactionStatus.Error &&
        <DetailRow>
          <DetailTitle />
          <StatusRow>
            <DetailButton onClick={onClickRetryTransaction}>{getLocale('braveWalletTransactionRetry')}</DetailButton>
          </StatusRow>
        </DetailRow>
      }
    </StyledWrapper>
  )
}

export default TransactionDetailPanel
