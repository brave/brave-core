import * as React from 'react'
import { reduceAddress } from '../../../utils/reduce-address'
import { formatBalance, formatFiatBalance } from '../../../utils/format-balances'
import { TransactionPopup } from '../'
import { create } from 'ethereum-blockies'
import {
  TransactionInfo,
  TransactionStatus,
  WalletAccountType,
  EthereumChain,
  TransactionType,
  TokenInfo,
  AssetPriceInfo
} from '../../../constants/types'
import locale from '../../../constants/locale'

// Styled Components
import {
  StyledWrapper,
  DetailRow,
  FromCircle,
  ToCircle,
  MoreButton,
  MoreIcon,
  DetailColumn,
  BalanceColumn,
  DetailTextLight,
  DetailTextDark,
  DetailTextDarkBold,
  ArrowIcon,
  StatusRow,
  TransactionDetailRow,
  StatusBubble
} from './style'

export interface Props {
  selectedNetwork: EthereumChain
  transaction: TransactionInfo
  account: WalletAccountType | undefined
  visibleTokens: TokenInfo[]
  transactionSpotPrices: AssetPriceInfo[]
}

const PortfolioTransactionItem = (props: Props) => {
  const { transaction, account, selectedNetwork, visibleTokens, transactionSpotPrices } = props
  const [showTransactionPopup, setShowTransactionPopup] = React.useState<boolean>(false)

  const findTokenInfo = (contractAddress: string) => {
    return visibleTokens.find((account) => account.contractAddress.toLowerCase() === contractAddress.toLowerCase())
  }

  const findSpotPrice = (symbol: string) => {
    return transactionSpotPrices.find((token) => token.fromAsset.toLowerCase() === symbol.toLowerCase())
  }

  const getTransactionDisplayInfo = (
    sentValue: string,
    sentDecimals: number,
    sentPrice: string,
    from: string,
    to: string
  ) => {
    const sentAmount = formatBalance(sentValue, sentDecimals)
    const sentFiatAmount = formatFiatBalance(sentValue, sentDecimals, sentPrice)
    const formatedFromAddress = reduceAddress(from)
    const formatedToAddress = reduceAddress(to)
    return {
      sentAmount,
      sentFiatAmount,
      formatedFromAddress,
      formatedToAddress
    }
  }

  const transactionInfo = React.useMemo(() => {
    const {
      fromAddress,
      txArgs,
      txStatus,
      txData,
      txType
    } = transaction
    const { baseData } = txData
    const { to, value } = baseData
    const ERC20Token = findTokenInfo(to)
    const ERC20TokenSymbol = ERC20Token?.symbol ?? ''
    const ERC20TokenDecimals = ERC20Token?.decimals ?? 18
    const ERC20TokenPrice = findSpotPrice(ERC20Token?.symbol ?? '')?.price ?? ''
    const networkPrice = findSpotPrice(selectedNetwork.symbol)?.price ?? ''
    if (txType === TransactionType.ERC20Transfer || txType === TransactionType.ERC20Approve) {
      const info = getTransactionDisplayInfo(
        txArgs[1],
        ERC20TokenDecimals,
        ERC20TokenPrice,
        fromAddress,
        txArgs[0]
      )
      return {
        status: txStatus,
        toAddress: txArgs[0],
        fromAddress: fromAddress,
        symbol: ERC20TokenSymbol,
        ...info
      }
    } else {
      const info = getTransactionDisplayInfo(
        value,
        selectedNetwork.decimals,
        networkPrice,
        fromAddress,
        to
      )
      return {
        status: txStatus,
        toAddress: to,
        fromAddress: fromAddress,
        symbol: selectedNetwork.symbol,
        ...info
      }
    }
  }, [transaction, transactionSpotPrices])

  const fromOrb = React.useMemo(() => {
    return create({ seed: transactionInfo.fromAddress.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [transactionInfo])

  const toOrb = React.useMemo(() => {
    return create({ seed: transactionInfo.toAddress.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [transactionInfo, transactionSpotPrices, visibleTokens])

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
      alert(locale.transactionExplorerMissing)
    }
  }

  return (
    <StyledWrapper onClick={onHideTransactionPopup}>
      <TransactionDetailRow>
        <FromCircle orb={fromOrb} />
        <ToCircle orb={toOrb} />
        <DetailColumn>
          <DetailRow>
            <DetailTextLight>{account?.name}</DetailTextLight>
            <DetailTextDark>{account?.address.toLowerCase() === transaction?.fromAddress.toLowerCase() ? locale.transactionSent : locale.transactionReceived}</DetailTextDark>
            <DetailTextDarkBold>{/*We need to return a Transaction Time Stamp to display date info here*/}</DetailTextDarkBold>
          </DetailRow>
          <DetailRow>
            <DetailTextDark>{transactionInfo.formatedFromAddress}</DetailTextDark>
            <ArrowIcon />
            <DetailTextDark>{transactionInfo.formatedToAddress}</DetailTextDark>
          </DetailRow>
        </DetailColumn>
      </TransactionDetailRow>
      <StatusRow>
        <StatusBubble status={transactionInfo.status} />
        <DetailTextDarkBold>{TransactionStatus[transactionInfo.status]}</DetailTextDarkBold>
      </StatusRow>
      <DetailRow>
        <BalanceColumn>
          <DetailTextDark>{/*We need to return a Transaction Time Stamp to calculate Fiat value here*/}${transactionInfo.sentFiatAmount}</DetailTextDark>
          <DetailTextLight>{transactionInfo.sentAmount} {transactionInfo.symbol}</DetailTextLight>
        </BalanceColumn>
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
