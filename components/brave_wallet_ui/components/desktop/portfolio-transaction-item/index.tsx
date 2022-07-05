import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'
import { useHistory } from 'react-router'
import * as EthereumBlockies from 'ethereum-blockies'

import {
  BraveWallet,
  WalletAccountType,
  WalletState,
  WalletRoutes
} from '../../../constants/types'

// Utils
import { toProperCase } from '../../../utils/string-utils'
import { formatDateAsRelative } from '../../../utils/datetime-utils'
import { mojoTimeDeltaToJSDate } from '../../../../common/mojomUtils'
import Amount from '../../../utils/amount'
import { copyToClipboard } from '../../../utils/copy-to-clipboard'
import { getCoinFromTxDataUnion } from '../../../utils/network-utils'
import { getLocale } from '../../../../common/locale'
import { isSolanaTransaction } from '../../../utils/tx-utils'

// Hooks
import { useExplorer, useTransactionParser } from '../../../common/hooks'
import { SwapExchangeProxy } from '../../../common/hooks/address-labels'
import { useTransactionsNetwork } from '../../../common/hooks/use-transactions-network'

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
import { WalletActions } from '../../../common/actions'

export interface Props {
  transaction: BraveWallet.TransactionInfo
  account: WalletAccountType | undefined
  accounts: WalletAccountType[]
  displayAccountName: boolean
}

export const PortfolioTransactionItem = ({
  transaction,
  account,
  displayAccountName,
  accounts
}: Props) => {
  // routing
  const history = useHistory()

  // redux
  const dispatch = useDispatch()
  const {
    defaultCurrencies,
    userVisibleTokensInfo
  } = useSelector(({ wallet }: { wallet: WalletState }) => wallet)

  // state
  const [showTransactionPopup, setShowTransactionPopup] = React.useState<boolean>(false)

  const isSolanaTxn = isSolanaTransaction(transaction)
  const isFilecoinTransaction = getCoinFromTxDataUnion(transaction.txDataUnion) === BraveWallet.CoinType.FIL

  // custom hooks
  const transactionsNetwork = useTransactionsNetwork(transaction)
  const onClickViewOnBlockExplorer = useExplorer(transactionsNetwork)
  const parseTransaction = useTransactionParser(transactionsNetwork)

  // methods
  const onShowTransactionPopup = () => setShowTransactionPopup(true)

  const onClickCopyTransactionHash = React.useCallback(
    () => copyToClipboard(transaction.txHash),
    [transaction.txHash]
  )

  const onClickRetryTransaction = React.useCallback(
    () => dispatch(WalletActions.retryTransaction(transaction)),
    [transaction]
  )

  const onClickSpeedupTransaction = React.useCallback(
    () => dispatch(WalletActions.speedupTransaction(transaction)),
    [transaction]
  )

  const onClickCancelTransaction = React.useCallback(
    () => dispatch(WalletActions.cancelTransaction(transaction)),
    [transaction]
  )

  const onHideTransactionPopup = React.useCallback(() => {
    if (showTransactionPopup) {
      setShowTransactionPopup(false)
    }
  }, [showTransactionPopup])

  const findWalletAccount = React.useCallback((address: string) => {
    return accounts.find((account) => account.address.toLowerCase() === address.toLowerCase())
  }, [accounts])

  const onSelectAccount = React.useCallback((account: WalletAccountType) => {
    history.push(`${WalletRoutes.Accounts}/${account.address}`)
  }, [history])

  const onAddressClick = React.useCallback((address?: string) => () => {
    if (!address) {
      return
    }

    const account = findWalletAccount(address)

    if (account !== undefined) {
      onSelectAccount(account)
      return
    }

    onClickViewOnBlockExplorer('address', address)
  }, [onSelectAccount, findWalletAccount, onClickViewOnBlockExplorer])

  const findToken = React.useCallback((symbol: string) => {
    return userVisibleTokensInfo.find((token) => token.symbol.toLowerCase() === symbol.toLowerCase())
  }, [userVisibleTokensInfo])

  const onSelectAsset = React.useCallback((asset: BraveWallet.BlockchainToken) => {
    if (asset.contractAddress === '') {
      history.push(`${WalletRoutes.Portfolio}/${asset.symbol}`)
      return
    }
    history.push(`${WalletRoutes.Portfolio}/${asset.contractAddress}`)
  }, [history])

  const onAssetClick = React.useCallback((symbol?: string) => () => {
    if (!symbol) {
      return
    }

    const asset = findToken(symbol)
    if (asset) {
      onSelectAsset(asset)
    }
  }, [onSelectAsset, findToken])

  // memos
  const transactionDetails = React.useMemo(
    () => parseTransaction(transaction),
    [transaction, parseTransaction]
  )

  const fromOrb = React.useMemo(() => {
    return EthereumBlockies.create({ seed: transactionDetails.sender.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [transactionDetails.sender])

  const toOrb = React.useMemo(() => {
    return EthereumBlockies.create({ seed: transactionDetails.recipient.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [transactionDetails.recipient])

  const transactionIntentDescription = React.useMemo(() => {
    switch (true) {
      case transaction.txType === BraveWallet.TransactionType.ERC20Approve: {
        const text = getLocale('braveWalletApprovalTransactionIntent')
        return (
          <DetailRow>
            <DetailTextDark>
              {toProperCase(text)} {
                transactionDetails.isApprovalUnlimited
                  ? getLocale('braveWalletTransactionApproveUnlimited')
                  : transactionDetails.value
              }{' '}
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

      case transaction.txType === BraveWallet.TransactionType.ETHSwap: {
        return (
          <DetailRow>
            <DetailTextDark>
              {transactionDetails.sellAmount}{' '}
              <AddressOrAsset
                onClick={onAssetClick(transactionDetails.sellToken?.symbol)}
              >
                {transactionDetails.sellToken?.symbol}
              </AddressOrAsset>
            </DetailTextDark>
            <ArrowIcon />
            <DetailTextDark>
              {transactionDetails.minBuyAmount}{' '}
              <AddressOrAsset onClick={onAddressClick(transactionDetails.buyToken?.symbol)}>
                {transactionDetails.buyToken?.symbol}
              </AddressOrAsset>
            </DetailTextDark>
          </DetailRow>
        )
      }

      case transaction.txType !== BraveWallet.TransactionType.ETHSwap && transactionDetails.isSwap: {
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
  }, [transactionDetails, onAssetClick, onAddressClick])

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
              disabled={
                transaction.txType === BraveWallet.TransactionType.ERC721TransferFrom ||
                transaction.txType === BraveWallet.TransactionType.ERC721SafeTransferFrom
              }
              onClick={onAssetClick(transactionDetails.symbol)}
            >
              {transactionDetails.symbol}
              {
                transaction.txType === BraveWallet.TransactionType.ERC721TransferFrom ||
                  transaction.txType === BraveWallet.TransactionType.ERC721SafeTransferFrom
                  ? ' ' + transactionDetails.erc721TokenId
                  : ''
              }
            </AddressOrAsset>
          </>
        )
      }
    }
  }, [transaction, transactionDetails, displayAccountName, onAssetClick])

  // render
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
          {transactionDetails.status === BraveWallet.TransactionStatus.Dropped && getLocale('braveWalletTransactionStatusDropped')}
        </DetailTextDarkBold>
      </StatusRow>
      <DetailRow>
        <BalanceColumn>
          <DetailTextDark>
            {/* We need to return a Transaction Time Stamp to calculate Fiat value here */}
            {transactionDetails.fiatValue
              .formatAsFiat(defaultCurrencies.fiat)}
          </DetailTextDark>
          <DetailTextLight>{transactionDetails.formattedNativeCurrencyTotal}</DetailTextLight>
        </BalanceColumn>
        {/* Will remove this conditional for solana once https://github.com/brave/brave-browser/issues/22040 is implemented. */}
        {!isSolanaTxn &&
          <TransactionFeesTooltip
            text={
              <>
                <TransactionFeeTooltipTitle>{getLocale('braveWalletAllowSpendTransactionFee')}</TransactionFeeTooltipTitle>
                <TransactionFeeTooltipBody>
                  {
                    new Amount(transactionDetails.gasFee)
                      .divideByDecimals(transactionsNetwork.decimals)
                      .formatAsAsset(6, transactionsNetwork.symbol)
                  }
                </TransactionFeeTooltipBody>
                <TransactionFeeTooltipBody>
                  {
                    new Amount(transactionDetails.gasFeeFiat)
                      .formatAsFiat(defaultCurrencies.fiat)
                  }
                </TransactionFeeTooltipBody>
              </>
            }
          >
            <CoinsButton>
              <CoinsIcon />
            </CoinsButton>
          </TransactionFeesTooltip>
        }

        {(transactionDetails.status !== BraveWallet.TransactionStatus.Rejected && transactionDetails.status !== BraveWallet.TransactionStatus.Unapproved) ? (
          <MoreButton onClick={onShowTransactionPopup}>
            <MoreIcon />
          </MoreButton>
        ) : (
          <RejectedTransactionSpacer />
        )}

        {showTransactionPopup &&
          <TransactionPopup>
            {[BraveWallet.TransactionStatus.Approved, BraveWallet.TransactionStatus.Submitted, BraveWallet.TransactionStatus.Confirmed, BraveWallet.TransactionStatus.Dropped].includes(transactionDetails.status) &&
              <TransactionPopupItem
                onClick={onClickViewOnBlockExplorer('tx', transaction.txHash)}
                text={getLocale('braveWalletTransactionExplorer')}
              />
            }

            {[BraveWallet.TransactionStatus.Approved, BraveWallet.TransactionStatus.Submitted, BraveWallet.TransactionStatus.Confirmed, BraveWallet.TransactionStatus.Dropped].includes(transactionDetails.status) &&
              <TransactionPopupItem
                onClick={onClickCopyTransactionHash}
                text={getLocale('braveWalletTransactionCopyHash')}
              />
            }

            {[BraveWallet.TransactionStatus.Submitted, BraveWallet.TransactionStatus.Approved].includes(transactionDetails.status) &&
              !isSolanaTxn &&
              !isFilecoinTransaction &&
              <TransactionPopupItem
                onClick={onClickSpeedupTransaction}
                text={getLocale('braveWalletTransactionSpeedup')}
              />
            }

            {[BraveWallet.TransactionStatus.Submitted, BraveWallet.TransactionStatus.Approved].includes(transactionDetails.status) &&
              !isSolanaTxn &&
              !isFilecoinTransaction &&
              <TransactionPopupItem
                onClick={onClickCancelTransaction}
                text={getLocale('braveWalletTransactionCancel')}
              />
            }

            {[BraveWallet.TransactionStatus.Error].includes(transactionDetails.status) &&
              !isSolanaTxn &&
              !isFilecoinTransaction &&
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
