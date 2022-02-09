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
import { mojoTimeDeltaToJSDate, formatDateAsRelative } from '../../../utils/datetime-utils'
import {
  formatFiatAmountWithCommasAndDecimals,
  formatTokenAmountWithCommasAndDecimals
} from '../../../utils/format-prices'
import { formatBalance } from '../../../utils/format-balances'

// Hooks
import { useExplorer, useTransactionParser } from '../../../common/hooks'
import { SwapExchangeProxy } from '../../../common/hooks/address-labels'

// Styled Components
import {
  AddressOrAsset,
  ArrowIcon,
  BalanceColumn,
  CoinsButton,
  CoinsIcon,
  DetailColumn,
  DetailRow,
  DetailTextDark,
  DetailTextDarkBold,
  DetailTextLight,
  RejectedTransactionSpacer,
  FromCircle,
  MoreButton,
  MoreIcon,
  StatusRow,
  StyledWrapper,
  ToCircle,
  TransactionDetailRow,
  TransactionFeeTooltipBody,
  TransactionFeeTooltipTitle
} from './style'
import { StatusBubble } from '../../shared/style'
import TransactionFeesTooltip from '../transaction-fees-tooltip'
import TransactionPopup, { TransactionPopupItem } from '../transaction-popup'
import TransactionTimestampTooltip from '../transaction-timestamp-tooltip'

export interface Props {
  selectedNetwork: BraveWallet.EthereumChain
  transaction: BraveWallet.TransactionInfo
  account: WalletAccountType | undefined
  accounts: WalletAccountType[]
  visibleTokens: BraveWallet.BlockchainToken[]
  transactionSpotPrices: BraveWallet.AssetPrice[]
  displayAccountName: boolean
  defaultCurrencies: DefaultCurrencies
  onSelectAccount: (account: WalletAccountType) => void
  onSelectAsset: (asset: BraveWallet.BlockchainToken) => void
  onRetryTransaction: (transaction: BraveWallet.TransactionInfo) => void
  onSpeedupTransaction: (transaction: BraveWallet.TransactionInfo) => void
  onCancelTransaction: (transaction: BraveWallet.TransactionInfo) => void
}

const PortfolioTransactionItem = (props: Props) => {
  const {
    transaction,
    account,
    selectedNetwork,
    visibleTokens,
    transactionSpotPrices,
    displayAccountName,
    accounts,
    defaultCurrencies,
    onSelectAccount,
    onSelectAsset,
    onRetryTransaction,
    onSpeedupTransaction,
    onCancelTransaction
  } = props
  const [showTransactionPopup, setShowTransactionPopup] = React.useState<boolean>(false)

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

  const onShowTransactionPopup = () => {
    setShowTransactionPopup(true)
  }

  const onHideTransactionPopup = () => {
    if (showTransactionPopup) {
      setShowTransactionPopup(false)
    }
  }

  const onClickViewOnBlockExplorer = useExplorer(selectedNetwork)

  const onClickRetryTransaction = () => {
    onRetryTransaction(transaction)
  }

  const onClickSpeedupTransaction = () => {
    onSpeedupTransaction(transaction)
  }

  const onClickCancelTransaction = () => {
    onCancelTransaction(transaction)
  }

  const findWalletAccount = React.useCallback((address: string) => {
    return accounts.find((account) => account.address.toLowerCase() === address.toLowerCase())
  }, [accounts])

  const onAddressClick = (address?: string) => () => {
    if (!address) {
      return
    }

    const account = findWalletAccount(address)

    if (account !== undefined) {
      onSelectAccount(account)
      return
    }

    onClickViewOnBlockExplorer('address', address)
  }

  const findToken = React.useCallback((symbol: string) => {
    return visibleTokens.find((token) => token.symbol.toLowerCase() === symbol.toLowerCase())
  }, [visibleTokens])

  const onAssetClick = (symbol?: string) => () => {
    if (!symbol) {
      return
    }

    const asset = findToken(symbol)
    if (asset) {
      onSelectAsset(asset)
    }
  }

  const transactionIntentLocale = React.useMemo(() => {
    switch (true) {
      case transaction.txType === BraveWallet.TransactionType.ERC20Approve: {
        const text = getLocale('braveWalletApprovalTransactionIntent')
        return (
          <>
            {displayAccountName ? text : toProperCase(text)}{' '}
            <AddressOrAsset onClick={onAssetClick(transactionDetails.symbol)}>
              {transactionDetails.symbol}
            </AddressOrAsset>
          </>
        )
      }

      // Detect sending to 0x Exchange Proxy
      case transaction.txDataUnion.ethTxData1559?.baseData.to.toLowerCase() === SwapExchangeProxy: {
        const text = getLocale('braveWalletSwap')
        return displayAccountName ? text.toLowerCase() : text
      }

      case transaction.txType === BraveWallet.TransactionType.ETHSend:
      case transaction.txType === BraveWallet.TransactionType.ERC20Transfer:
      case transaction.txType === BraveWallet.TransactionType.ERC721TransferFrom:
      case transaction.txType === BraveWallet.TransactionType.ERC721SafeTransferFrom:
      default: {
        const text = getLocale('braveWalletTransactionSent')
        return (
          <>
            {displayAccountName ? text : toProperCase(text)}{' '}
            <AddressOrAsset
              // Disabled for ERC721 tokens until we have NFT meta data
              disabled={transaction.txType === BraveWallet.TransactionType.ERC721TransferFrom || transaction.txType === BraveWallet.TransactionType.ERC721SafeTransferFrom}
              onClick={onAssetClick(transactionDetails.symbol)}
            >
              {transactionDetails.symbol}
              {transaction.txType === BraveWallet.TransactionType.ERC721TransferFrom || transaction.txType === BraveWallet.TransactionType.ERC721SafeTransferFrom
                ? ' ' + transactionDetails.erc721TokenId : ''}
            </AddressOrAsset>
          </>
        )
      }
    }
  }, [transaction])

  const transactionIntentDescription = React.useMemo(() => {
    switch (true) {
      case transaction.txType === BraveWallet.TransactionType.ERC20Approve: {
        const text = getLocale('braveWalletApprovalTransactionIntent')
        return (
          <DetailRow>
            <DetailTextDark>
              {toProperCase(text)} {transactionDetails.value}{' '}
              <AddressOrAsset onClick={onAssetClick(transactionDetails.symbol)}>
                {transactionDetails.symbol}
              </AddressOrAsset> -{' '}
              <AddressOrAsset onClick={onAddressClick(transactionDetails.approvalTarget)}>
                {transactionDetails.approvalTargetLabel}
              </AddressOrAsset>
            </DetailTextDark>
          </DetailRow>
        )
      }

      // FIXME: Add as new BraveWallet.TransactionType on the service side.
      case transaction.txDataUnion.ethTxData1559?.baseData.to.toLowerCase() === SwapExchangeProxy: {
        return (
          <DetailRow>
            <DetailTextDark>
              {transactionDetails.value}{' '}
              <AddressOrAsset onClick={onAssetClick(transactionDetails.symbol)}>
                {transactionDetails.symbol}
              </AddressOrAsset>
            </DetailTextDark>
            <ArrowIcon />
            <DetailTextDark>
              <AddressOrAsset onClick={onAddressClick(transactionDetails.recipient)}>
                {transactionDetails.recipientLabel}
              </AddressOrAsset>
            </DetailTextDark>
          </DetailRow>
        )
      }

      case transaction.txType === BraveWallet.TransactionType.ETHSend:
      case transaction.txType === BraveWallet.TransactionType.ERC20Transfer:
      case transaction.txType === BraveWallet.TransactionType.ERC721TransferFrom:
      case transaction.txType === BraveWallet.TransactionType.ERC721SafeTransferFrom:
      default: {
        return (
          <DetailRow>
            <DetailTextDark>
              <AddressOrAsset onClick={onAddressClick(transactionDetails.sender)}>
                {transactionDetails.senderLabel}
              </AddressOrAsset>
            </DetailTextDark>
            <ArrowIcon />
            <DetailTextDark>
              <AddressOrAsset onClick={onAddressClick(transactionDetails.recipient)}>
                {transactionDetails.recipientLabel}
              </AddressOrAsset>
            </DetailTextDark>
          </DetailRow>
        )
      }
    }
  }, [transactionDetails])

  return (
    <StyledWrapper onClick={onHideTransactionPopup}>
      <TransactionDetailRow>
        <FromCircle orb={fromOrb} />
        <ToCircle orb={toOrb} />
        <DetailColumn>
          <DetailRow>
            { // Display account name only if rendered under Portfolio view
              displayAccountName &&
              <DetailTextLight>
                {account?.name}
              </DetailTextLight>
            }
            <DetailTextDark>
              {transactionIntentLocale}
            </DetailTextDark>
            <DetailTextLight>-</DetailTextLight>

            <TransactionTimestampTooltip
              text={
                <TransactionFeeTooltipBody>
                  {mojoTimeDeltaToJSDate(transactionDetails.createdTime).toUTCString()}
                </TransactionFeeTooltipBody>
              }
            >
              <DetailTextDarkBold>
                {formatDateAsRelative(mojoTimeDeltaToJSDate(transactionDetails.createdTime))}
              </DetailTextDarkBold>
            </TransactionTimestampTooltip>
          </DetailRow>
          {transactionIntentDescription}
        </DetailColumn>
      </TransactionDetailRow>
      <StatusRow>
        <StatusBubble status={transactionDetails.status} />
        <DetailTextDarkBold>
          {transactionDetails.status === BraveWallet.TransactionStatus.Unapproved && getLocale('braveWalletTransactionStatusUnapproved')}
          {transactionDetails.status === BraveWallet.TransactionStatus.Approved && getLocale('braveWalletTransactionStatusApproved')}
          {transactionDetails.status === BraveWallet.TransactionStatus.Rejected && getLocale('braveWalletTransactionStatusRejected')}
          {transactionDetails.status === BraveWallet.TransactionStatus.Submitted && getLocale('braveWalletTransactionStatusSubmitted')}
          {transactionDetails.status === BraveWallet.TransactionStatus.Confirmed && getLocale('braveWalletTransactionStatusConfirmed')}
          {transactionDetails.status === BraveWallet.TransactionStatus.Error && getLocale('braveWalletTransactionStatusError')}
        </DetailTextDarkBold>
      </StatusRow>
      <DetailRow>
        <BalanceColumn>
          <DetailTextDark>{/* We need to return a Transaction Time Stamp to calculate Fiat value here */}{formatFiatAmountWithCommasAndDecimals(transactionDetails.fiatValue, defaultCurrencies.fiat)}</DetailTextDark>
          <DetailTextLight>{formatTokenAmountWithCommasAndDecimals(transactionDetails.nativeCurrencyTotal, selectedNetwork.symbol)}</DetailTextLight>
        </BalanceColumn>
        <TransactionFeesTooltip
          text={
            <>
              <TransactionFeeTooltipTitle>{getLocale('braveWalletAllowSpendTransactionFee')}</TransactionFeeTooltipTitle>
              <TransactionFeeTooltipBody>{formatTokenAmountWithCommasAndDecimals(formatBalance(transactionDetails.gasFee, selectedNetwork.decimals), selectedNetwork.symbol)}</TransactionFeeTooltipBody>
              <TransactionFeeTooltipBody>{formatFiatAmountWithCommasAndDecimals(transactionDetails.gasFeeFiat, defaultCurrencies.fiat)}</TransactionFeeTooltipBody>
            </>
          }
        >
          <CoinsButton>
            <CoinsIcon />
          </CoinsButton>
        </TransactionFeesTooltip>

        {transactionDetails.status !== BraveWallet.TransactionStatus.Rejected ? (
          <MoreButton onClick={onShowTransactionPopup}>
            <MoreIcon />
          </MoreButton>
        ) : (
          <RejectedTransactionSpacer />
        )}

        {showTransactionPopup &&
          <TransactionPopup>
            {[BraveWallet.TransactionStatus.Approved, BraveWallet.TransactionStatus.Submitted, BraveWallet.TransactionStatus.Confirmed] &&
              <TransactionPopupItem
                onClick={onClickViewOnBlockExplorer('tx', transaction.txHash)}
                text={getLocale('braveWalletTransactionExplorer')}
              />
            }

            {[BraveWallet.TransactionStatus.Submitted, BraveWallet.TransactionStatus.Approved].includes(transactionDetails.status) &&
              <TransactionPopupItem
                onClick={onClickSpeedupTransaction}
                text={getLocale('braveWalletTransactionSpeedup')}
              />
            }

            {[BraveWallet.TransactionStatus.Submitted, BraveWallet.TransactionStatus.Approved].includes(transactionDetails.status) &&
              <TransactionPopupItem
                onClick={onClickCancelTransaction}
                text={getLocale('braveWalletTransactionCancel')}
              />
            }

            {[BraveWallet.TransactionStatus.Error].includes(transactionDetails.status) &&
              <TransactionPopupItem
                onClick={onClickRetryTransaction}
                text={getLocale('braveWalletTransactionRetry')}
              />
            }
          </TransactionPopup>
        }
      </DetailRow>
    </StyledWrapper>
  )
}

export default PortfolioTransactionItem
