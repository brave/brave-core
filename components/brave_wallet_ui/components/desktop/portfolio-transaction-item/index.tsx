import * as React from 'react'
import * as EthereumBlockies from 'ethereum-blockies'

import { getLocale } from '../../../../common/locale'
import { toProperCase } from '../../../utils/string-utils'
import { TransactionPopup } from '../'
import {
  AssetPriceInfo,
  EthereumChain,
  TokenInfo,
  TransactionInfo,
  TransactionStatus,
  TransactionType,
  WalletAccountType
} from '../../../constants/types'
import { useTransactionParser } from '../../../common/hooks'
import { SwapExchangeProxy } from '../../../common/hooks/address-labels'

// Styled Components
import {
  AddressOrAsset,
  ArrowIcon,
  BalanceColumn, CoinsButton, CoinsIcon,
  DetailColumn,
  DetailRow,
  DetailTextDark,
  DetailTextDarkBold,
  DetailTextLight,
  FromCircle,
  MoreButton,
  MoreIcon,
  StatusBubble,
  StatusRow,
  StyledWrapper,
  ToCircle,
  TransactionDetailRow, TransactionFeeTooltipBody, TransactionFeeTooltipTitle
} from './style'
import TransactionFeesTooltip from '../transaction-fees-tooltip'

export interface Props {
  selectedNetwork: EthereumChain
  transaction: TransactionInfo
  account: WalletAccountType | undefined
  accounts: WalletAccountType[]
  visibleTokens: TokenInfo[]
  transactionSpotPrices: AssetPriceInfo[]
  displayAccountName: boolean
  onSelectAccount: (account: WalletAccountType) => void
  onSelectAsset: (asset: TokenInfo) => void
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
    onSelectAccount,
    onSelectAsset
  } = props
  const [showTransactionPopup, setShowTransactionPopup] = React.useState<boolean>(false)

  const parseTransaction = useTransactionParser(selectedNetwork, accounts, transactionSpotPrices, visibleTokens)
  const transactionDetails = parseTransaction(transaction)

  const fromOrb = React.useMemo(() => {
    return EthereumBlockies.create({ seed: transactionDetails.sender.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [transactionDetails])

  const toOrb = React.useMemo(() => {
    return EthereumBlockies.create({ seed: transactionDetails.recipient.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [transactionDetails, transactionSpotPrices, visibleTokens])

  const onShowTransactionPopup = () => {
    setShowTransactionPopup(true)
  }

  const onHideTransactionPopup = () => {
    if (showTransactionPopup) {
      setShowTransactionPopup(false)
    }
  }

  const onClickViewOnBlockExplorer = () => {
    const exporerURL = selectedNetwork.blockExplorerUrls[0]
    if (exporerURL && transaction.txHash) {
      const url = `${exporerURL}/tx/${transaction.txHash}`
      window.open(url, '_blank')
    } else {
      alert(getLocale('braveWalletTransactionExplorerMissing'))
    }
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

    const explorerUrl = selectedNetwork.blockExplorerUrls[0]
    if (explorerUrl) {
      const url = `${explorerUrl}/address/${address}`
      window.open(url, '_blank')
    } else {
      alert(getLocale('braveWalletTransactionExplorerMissing'))
    }
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
      case transaction.txType === TransactionType.ERC20Approve: {
        const text = getLocale('braveWalletApprovalTransactionIntent')
        return (
          <>
            {displayAccountName ? text : toProperCase(text)}{` `}
            <AddressOrAsset onClick={onAssetClick(transactionDetails.symbol)}>
              {transactionDetails.symbol}
            </AddressOrAsset>
          </>
        )
      }

      // Detect sending to 0x Exchange Proxy
      case transaction.txData.baseData.to.toLowerCase() === SwapExchangeProxy: {
        const text = getLocale('braveWalletSwap')
        return displayAccountName ? text.toLowerCase() : text
      }

      case transaction.txType === TransactionType.ETHSend:
      case transaction.txType === TransactionType.ERC20Transfer:
      default: {
        const text = getLocale('braveWalletTransactionSent')
        return (
            <>
              {displayAccountName ? text : toProperCase(text)}{` `}
              <AddressOrAsset onClick={onAssetClick(transactionDetails.symbol)}>
                {transactionDetails.symbol}
              </AddressOrAsset>
            </>
        )
      }
    }
  }, [transaction])

  const transactionIntentDescription = React.useMemo(() => {
    switch (true) {
      case transaction.txType === TransactionType.ERC20Approve: {
        const text = getLocale('braveWalletApprovalTransactionIntent')
        return (
          <DetailRow>
            <DetailTextDark>
              {displayAccountName ? text : toProperCase(text)}{` `}
              {transactionDetails.value}{` `}
              <AddressOrAsset onClick={onAssetClick(transactionDetails.symbol)}>
                {transactionDetails.symbol}
              </AddressOrAsset> -{` `}
              <AddressOrAsset onClick={onAddressClick(transactionDetails.approvalTarget)}>
                {transactionDetails.approvalTargetLabel}
              </AddressOrAsset>
            </DetailTextDark>
          </DetailRow>
        )
      }

      // FIXME: Add as new TransactionType on the controller side.
      case transaction.txData.baseData.to.toLowerCase() === SwapExchangeProxy: {
        return (
          <DetailRow>
            <DetailTextDark>
              {displayAccountName && (
                <AddressOrAsset onClick={onAddressClick(transactionDetails.sender)}>
                  {transactionDetails.senderLabel}
                </AddressOrAsset>
              )}
              {displayAccountName && ` `}
              <AddressOrAsset onClick={onAssetClick(transactionDetails.symbol)}>
                {transactionDetails.symbol}
              </AddressOrAsset>
              {` `}
              ({transactionDetails.value})
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

      case transaction.txType === TransactionType.ETHSend:
      case transaction.txType === TransactionType.ERC20Transfer:
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
            <DetailTextDarkBold>{/*We need to return a Transaction Time Stamp to display date info here*/}</DetailTextDarkBold>
          </DetailRow>
          {transactionIntentDescription}
        </DetailColumn>
      </TransactionDetailRow>
      <StatusRow>
        <StatusBubble status={transactionDetails.status} />
        <DetailTextDarkBold>{TransactionStatus[transactionDetails.status]}</DetailTextDarkBold>
      </StatusRow>
      <DetailRow>
        <BalanceColumn>
          <DetailTextDark>{/*We need to return a Transaction Time Stamp to calculate Fiat value here*/}${transactionDetails.fiatValue}</DetailTextDark>
          <DetailTextLight>{transactionDetails.nativeCurrencyTotal} {selectedNetwork.symbol}</DetailTextLight>
        </BalanceColumn>
        <TransactionFeesTooltip
          text={
            <>
              <TransactionFeeTooltipTitle>Transaction fee</TransactionFeeTooltipTitle>
              <TransactionFeeTooltipBody>{transactionDetails.gasFee} {selectedNetwork.symbol}</TransactionFeeTooltipBody>
              <TransactionFeeTooltipBody>${transactionDetails.gasFeeFiat}</TransactionFeeTooltipBody>
            </>
          }
        >
          <CoinsButton>
            <CoinsIcon />
          </CoinsButton>
        </TransactionFeesTooltip>
        <MoreButton onClick={onShowTransactionPopup}>
          <MoreIcon />
        </MoreButton>
        {showTransactionPopup &&
          <TransactionPopup onClickView={onClickViewOnBlockExplorer} />
        }
      </DetailRow>
    </StyledWrapper>
  )
}

export default PortfolioTransactionItem
