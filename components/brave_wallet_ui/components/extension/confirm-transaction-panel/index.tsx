import * as React from 'react'
import { create } from 'ethereum-blockies'
import {
  WalletAccountType,
  EthereumChain,
  TransactionInfo,
  TransactionType,
  AssetPriceInfo,
  TokenInfo
} from '../../../constants/types'
import { reduceAddress } from '../../../utils/reduce-address'
import { reduceNetworkDisplayName } from '../../../utils/network-utils'
import locale from '../../../constants/locale'
import { formatBalance, formatFiatBalance } from '../../../utils/format-balances'
import { NavButton, PanelTab, TransactionDetailBox } from '../'

// Styled Components
import {
  StyledWrapper,
  FromCircle,
  ToCircle,
  AccountNameText,
  TopRow,
  NetworkText,
  TransactionAmmountBig,
  TransactionFiatAmountBig,
  GrandTotalText,
  MessageBox,
  TransactionTitle,
  TransactionTypeText,
  TransactionText,
  ButtonRow,
  AccountCircleWrapper,
  ArrowIcon,
  FromToRow,
  Divider,
  SectionRow,
  SectionRightColumn,
  EditButton,
  MessageBoxRow,
  FiatRow,
  FavIcon,
  URLText
} from './style'

import {
  TabRow,
  Description,
  PanelTitle,
  AccountCircle,
  AddressAndOrb,
  AddressText
} from '../shared-panel-styles'

export type confirmPanelTabs = 'transaction' | 'details'

export interface Props {
  accounts: WalletAccountType[]
  visibleTokens: TokenInfo[]
  transactionInfo: TransactionInfo
  selectedNetwork: EthereumChain
  transactionSpotPrices: AssetPriceInfo[]
  onConfirm: () => void
  onReject: () => void
}

function ConfirmTransactionPanel (props: Props) {
  const {
    accounts,
    // selectedNetwork,
    transactionInfo,
    visibleTokens,
    transactionSpotPrices,
    onConfirm,
    onReject
  } = props

  // TODO: Remove this once selectedNetwork is return correct decimal's and symbol's
  const selectedNetwork = {
    symbol: 'ETH',
    decimals: 18,
    chainName: 'Ethereum'
  }

  const [selectedTab, setSelectedTab] = React.useState<confirmPanelTabs>('transaction')

  const findTokenInfo = (contractAddress: string) => {
    return visibleTokens.find((account) => account.contractAddress.toLowerCase() === contractAddress.toLowerCase())
  }

  const findSpotPrice = (symbol: string) => {
    return transactionSpotPrices.find((token) => token.fromAsset.toLowerCase() === symbol.toLowerCase())
  }

  const transaction = React.useMemo(() => {
    const { txType, txArgs } = transactionInfo
    const { baseData } = transactionInfo.txData
    const { gasPrice, value, data, to } = baseData
    const selectedNetworkPrice = findSpotPrice(selectedNetwork.symbol)?.price
    if (txType === TransactionType.ETHSend) {
      const sendAmount = formatBalance(value, selectedNetwork.decimals)
      const sendFiatAmount = formatFiatBalance(value, selectedNetwork.decimals, selectedNetworkPrice ?? '')
      const gasAmount = formatBalance(gasPrice, selectedNetwork.decimals)
      const gasFiatAmount = formatFiatBalance(gasPrice, selectedNetwork.decimals, selectedNetworkPrice ?? '')
      const grandTotalFiatAmount = Number(sendFiatAmount) + Number(gasFiatAmount)
      return {
        sendAmount,
        sendTo: to,
        sendFiatAmount,
        gasAmount,
        gasFiatAmount,
        grandTotalFiatAmount,
        symbol: selectedNetwork.symbol,
        hasNoData: data.length === 0
      }
    } else if (txType === TransactionType.ERC20Transfer) {
      const ERC20Token = findTokenInfo(to)
      const ERC20TokenPrice = findSpotPrice(ERC20Token?.symbol ?? '')?.price
      const sendAmount = formatBalance(txArgs[1], ERC20Token?.decimals ?? 18)
      const sendFiatAmount = formatFiatBalance(txArgs[1], ERC20Token?.decimals ?? 18, ERC20TokenPrice ?? '')
      const gasAmount = formatBalance(gasPrice, selectedNetwork.decimals)
      const gasFiatAmount = formatFiatBalance(gasPrice, selectedNetwork.decimals, selectedNetworkPrice ?? '')
      const grandTotalFiatAmount = Number(sendFiatAmount) + Number(gasFiatAmount)
      return {
        sendAmount,
        sendTo: txArgs[0],
        sendFiatAmount,
        gasAmount,
        gasFiatAmount,
        grandTotalFiatAmount,
        symbol: ERC20Token?.symbol ?? '',
        hasNoData: data.length === 0
      }
    } else if (txType === TransactionType.ERC20Approve) {
      const ERC20Token = findTokenInfo(to)
      const ERC20TokenPrice = findSpotPrice(ERC20Token?.symbol ?? '')?.price
      const sendAmount = formatBalance(txArgs[1], ERC20Token?.decimals ?? 18)
      const sendFiatAmount = formatFiatBalance(txArgs[1], ERC20Token?.decimals ?? 18, ERC20TokenPrice ?? '')
      const gasAmount = formatBalance(gasPrice, selectedNetwork.decimals)
      const gasFiatAmount = formatFiatBalance(gasPrice, selectedNetwork.decimals, selectedNetworkPrice ?? '')
      const grandTotalFiatAmount = Number(sendFiatAmount) + Number(gasFiatAmount)
      return {
        sendAmount,
        sendTo: txArgs[0],
        sendFiatAmount,
        gasAmount,
        gasFiatAmount,
        grandTotalFiatAmount,
        symbol: ERC20Token?.symbol ?? '',
        hasNoData: data.length === 0,
        siteURL: 'https://app.compound.finance'
      }
    } else {
      const sendAmount = formatBalance(value, selectedNetwork.decimals)
      const sendFiatAmount = formatFiatBalance(value, selectedNetwork.decimals, selectedNetworkPrice ?? '')
      const gasAmount = formatBalance(gasPrice, selectedNetwork.decimals)
      const gasFiatAmount = formatFiatBalance(gasPrice, selectedNetwork.decimals, selectedNetworkPrice ?? '')
      const grandTotalFiatAmount = Number(sendFiatAmount) + Number(gasFiatAmount)
      return {
        sendAmount,
        sendTo: to,
        sendFiatAmount,
        gasAmount,
        gasFiatAmount,
        grandTotalFiatAmount,
        symbol: selectedNetwork.symbol,
        hasNoData: data.length === 0
      }
    }
  }, [transactionInfo, transactionSpotPrices])

  const onSelectTab = (tab: confirmPanelTabs) => () => {
    setSelectedTab(tab)
  }

  const findAccountName = (address: string) => {
    return accounts.find((account) => account.address.toLowerCase() === address.toLowerCase())?.name
  }

  const fromOrb = React.useMemo(() => {
    return create({ seed: transactionInfo.fromAddress, size: 8, scale: 16 }).toDataURL()
  }, [transactionInfo])

  const toOrb = React.useMemo(() => {
    return create({ seed: transaction.sendTo, size: 8, scale: 10 }).toDataURL()
  }, [transactionInfo])

  return (
    <StyledWrapper>
      <TopRow>
        <NetworkText>{reduceNetworkDisplayName(selectedNetwork.chainName)}</NetworkText>
        {transactionInfo.txType === TransactionType.ERC20Approve &&
          <AddressAndOrb>
            <AddressText>{reduceAddress(transaction.sendTo)}</AddressText>
            <AccountCircle orb={toOrb} />
          </AddressAndOrb>
        }
      </TopRow>
      {transactionInfo.txType === TransactionType.ERC20Approve ? (
        <>
          <FavIcon src={`chrome://favicon/size/64@1x/${transaction.siteURL}`} />
          <URLText>{transaction.siteURL}</URLText>
          <PanelTitle>{locale.allowSpendTitle} {transaction.symbol}?</PanelTitle>
          {/* Will need to allow parameterized locales by introducing the "t" helper. For ex: {t(locale.allowSpendDescription, [spendPayload.erc20Token.symbol])}*/}
          <Description>{locale.allowSpendDescriptionFirstHalf}{transaction.symbol}{locale.allowSpendDescriptionSecondHalf}</Description>
        </>
      ) : (
        <>
          <AccountCircleWrapper>
            <FromCircle orb={fromOrb} />
            <ToCircle orb={toOrb} />
          </AccountCircleWrapper>
          <FromToRow>
            <AccountNameText>{findAccountName(transactionInfo.fromAddress)}</AccountNameText>
            <ArrowIcon />
            <AccountNameText>{reduceAddress(transaction.sendTo)}</AccountNameText>
          </FromToRow>
          <TransactionTypeText>Send</TransactionTypeText>
          <TransactionAmmountBig>{transaction.sendAmount} {transaction.symbol}</TransactionAmmountBig>
          <TransactionFiatAmountBig>${transaction.sendFiatAmount}</TransactionFiatAmountBig>
        </>
      )}
      <TabRow>
        <PanelTab
          isSelected={selectedTab === 'transaction'}
          onSubmit={onSelectTab('transaction')}
          text='Transaction'
        />
        <PanelTab
          isSelected={selectedTab === 'details'}
          onSubmit={onSelectTab('details')}
          text='Details'
        />
      </TabRow>
      <MessageBox isDetails={selectedTab === 'details'} isApprove={transactionInfo.txType === 2}>
        {selectedTab === 'transaction' ? (
          <>
            {transactionInfo.txType === TransactionType.ERC20Approve ? (
              <>
                <MessageBoxRow>
                  <TransactionTitle>{locale.allowSpendTransactionFee}</TransactionTitle>
                  <EditButton>{locale.allowSpendEditButton}</EditButton>
                </MessageBoxRow>
                <FiatRow>
                  <TransactionTypeText>{transaction.gasAmount} {selectedNetwork.symbol}</TransactionTypeText>
                </FiatRow>
                <FiatRow>
                  <TransactionText>${transaction.gasFiatAmount}</TransactionText>
                </FiatRow>
              </>
            ) : (
              <>
                <SectionRow>
                  <TransactionTitle>{locale.confirmTransactionGasFee}</TransactionTitle>
                  <SectionRightColumn>
                    <EditButton>{locale.allowSpendEditButton}</EditButton>
                    <TransactionTypeText>{transaction.gasAmount} {selectedNetwork.symbol}</TransactionTypeText>
                    <TransactionText>${transaction.gasFiatAmount}</TransactionText>
                  </SectionRightColumn>
                </SectionRow>
                <Divider />
                <SectionRow>
                  <TransactionTitle>{locale.confirmTransactionTotal}</TransactionTitle>
                  <SectionRightColumn>
                    <TransactionText>{locale.confirmTransactionAmountGas}</TransactionText>
                    <GrandTotalText>{transaction.sendAmount} {transaction.symbol} + {transaction.gasAmount} {selectedNetwork.symbol}</GrandTotalText>
                    <TransactionText>${transaction.grandTotalFiatAmount.toFixed(2)}</TransactionText>
                  </SectionRightColumn>
                </SectionRow>
              </>
            )}
          </>
        ) : (
          <TransactionDetailBox
            hasNoData={transaction.hasNoData}
            transactionInfo={transactionInfo}
          />
        )}
      </MessageBox>
      <ButtonRow>
        <NavButton
          buttonType='reject'
          text={locale.allowSpendRejectButton}
          onSubmit={onReject}
        />
        <NavButton
          buttonType='confirm'
          text={locale.allowSpendConfirmButton}
          onSubmit={onConfirm}
        />
      </ButtonRow>
    </StyledWrapper>
  )
}

export default ConfirmTransactionPanel
