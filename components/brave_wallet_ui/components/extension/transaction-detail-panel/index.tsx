import * as React from 'react'
import { useSelector } from 'react-redux'
import * as EthereumBlockies from 'ethereum-blockies'

// Hooks
import { useExplorer, useTransactionParser } from '../../../common/hooks'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import { getTransactionStatusString } from '../../../utils/tx-utils'
import { toProperCase } from '../../../utils/string-utils'
import { mojoTimeDeltaToJSDate } from '../../../../common/mojomUtils'
import Amount from '../../../utils/amount'

import { getLocale } from '../../../../common/locale'
import {
  BraveWallet,
  WalletAccountType,
  DefaultCurrencies,
  WalletState
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
  ArrowIcon,
  AlertIcon
} from './style'

import {
  DetailTextDarkBold,
  DetailTextDark
} from '../shared-panel-styles'

import { StatusBubble } from '../../shared/style'
import { TransactionStatusTooltip } from '../transaction-status-tooltip'

export interface Props {
  transaction: BraveWallet.TransactionInfo
  selectedNetwork: BraveWallet.NetworkInfo
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
  // props
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

  // redux
  const {
    selectedAccount,
    transactions,
    transactionProviderErrorRegistry
  } = useSelector((state: { wallet: WalletState }) => state.wallet)

  const liveTransaction = React.useMemo(() => {
    return selectedAccount?.address && transactions[selectedAccount.address]
      ? transactions[selectedAccount.address].find(tx => tx.id === transaction.id) ?? transaction
      : transaction
  }, [transaction, selectedAccount, transactions])

  const parseTransaction = useTransactionParser(selectedNetwork, accounts, transactionSpotPrices, visibleTokens)
  const transactionDetails = React.useMemo(
    () => parseTransaction(liveTransaction),
    [liveTransaction]
  )

  const fromOrb = React.useMemo(() => {
    return EthereumBlockies.create({ seed: transactionDetails.sender.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [transactionDetails.sender])

  const toOrb = React.useMemo(() => {
    return EthereumBlockies.create({ seed: transactionDetails.recipient.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [transactionDetails.recipient])

  const onClickViewOnBlockExplorer = useExplorer(selectedNetwork)

  const onClickRetryTransaction = () => {
    if (liveTransaction) {
      onRetryTransaction(liveTransaction)
    }
  }

  const onClickSpeedupTransaction = () => {
    if (liveTransaction) {
      onSpeedupTransaction(liveTransaction)
    }
  }

  const onClickCancelTransaction = () => {
    if (liveTransaction) {
      onCancelTransaction(liveTransaction)
    }
  }

  const transactionTitle = React.useMemo((): string => {
    if (transactionDetails.isSwap) {
      return toProperCase(getLocale('braveWalletSwap'))
    }
    if (liveTransaction.txType === BraveWallet.TransactionType.ERC20Approve) {
      return toProperCase(getLocale('braveWalletApprovalTransactionIntent'))
    }
    return toProperCase(getLocale('braveWalletTransactionSent'))
  }, [transactionDetails, liveTransaction])

  const transactionValue = React.useMemo((): string => {
    if (liveTransaction.txType === BraveWallet.TransactionType.ERC721TransferFrom ||
        liveTransaction.txType === BraveWallet.TransactionType.ERC721SafeTransferFrom) {
      return transactionDetails.erc721BlockchainToken?.name + ' ' + transactionDetails.erc721TokenId
    }
    return new Amount(transactionDetails.value)
      .formatAsAsset(undefined, transactionDetails.symbol)
  }, [transactionDetails, liveTransaction])

  const transactionFiatValue = React.useMemo((): string => {
    if (liveTransaction.txType !== BraveWallet.TransactionType.ERC721TransferFrom &&
      liveTransaction.txType !== BraveWallet.TransactionType.ERC721SafeTransferFrom &&
      liveTransaction.txType !== BraveWallet.TransactionType.ERC20Approve) {
      return transactionDetails.fiatValue
        .formatAsFiat(defaultCurrencies.fiat)
    }
    return ''
  }, [transactionDetails, liveTransaction, defaultCurrencies])

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
          {getLocale('braveWalletTransactionDetailStatus')}
        </DetailTitle>
        <StatusRow>
          <StatusBubble status={transactionDetails.status} />
          <DetailTextDarkBold>
            {getTransactionStatusString(transactionDetails.status)}
          </DetailTextDarkBold>

          {transactionDetails.status === BraveWallet.TransactionStatus.Error && transactionProviderErrorRegistry[liveTransaction.id] &&
              <TransactionStatusTooltip text={
                `${transactionProviderErrorRegistry[liveTransaction.id].code}: ${transactionProviderErrorRegistry[liveTransaction.id].message}`
              }>
                <AlertIcon />
              </TransactionStatusTooltip>
          }
        </StatusRow>
      </DetailRow>
      <DetailRow>
        <DetailTitle>
          {getLocale('braveWalletAllowSpendTransactionFee')}
        </DetailTitle>
        <BalanceColumn>
          <DetailTextDark>
            {
              new Amount(transactionDetails.gasFee)
                .divideByDecimals(selectedNetwork.decimals)
                .formatAsAsset(6, selectedNetwork.symbol)
            }
          </DetailTextDark>
          <DetailTextDark>
            {
              new Amount(transactionDetails.gasFeeFiat)
                .formatAsFiat(defaultCurrencies.fiat)
            }
          </DetailTextDark>
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
      {![BraveWallet.TransactionStatus.Rejected, BraveWallet.TransactionStatus.Error].includes(transactionDetails.status) &&
        <DetailRow>
          <DetailTitle>
            {getLocale('braveWalletTransactionDetailHash')}
          </DetailTitle>
          <DetailButton onClick={onClickViewOnBlockExplorer('tx', liveTransaction?.txHash)}>
            {reduceAddress(liveTransaction.txHash)}
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

      {[BraveWallet.TransactionStatus.Approved, BraveWallet.TransactionStatus.Submitted].includes(transactionDetails.status) &&
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
