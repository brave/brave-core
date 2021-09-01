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
  EditButton
} from './style'

import { TabRow } from '../shared-panel-styles'

export type confirmPanelTabs = 'transaction' | 'details'

export interface Props {
  accounts: WalletAccountType[]
  visibleTokens: TokenInfo[]
  transactionInfo: TransactionInfo
  selectedNetwork: EthereumChain
  getTokenPrice: (symbol: string) => AssetPriceInfo
  onConfirm: () => void
  onReject: () => void
}

function ConfirmTransactionPanel (props: Props) {
  const {
    accounts,
    // selectedNetwork,
    transactionInfo,
    visibleTokens,
    getTokenPrice,
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

  const fromOrb = React.useMemo(() => {
    return create({ seed: transactionInfo.fromAddress, size: 8, scale: 16 }).toDataURL()
  }, [transactionInfo])

  const toOrb = React.useMemo(() => {
    return create({ seed: transactionInfo.txData.baseData.to, size: 8, scale: 10 }).toDataURL()
  }, [transactionInfo])

  const findTokenInfo = (contractAddress: string) => {
    return visibleTokens.find((account) => account.contractAddress.toLowerCase() === contractAddress.toLowerCase())
  }

  const transaction = React.useMemo(() => {
    const { txType, txArgs } = transactionInfo
    const { baseData } = transactionInfo.txData
    const { gasPrice, value, data, to } = baseData
    const selectedNetworkPrice = getTokenPrice(selectedNetwork.symbol).price
    if (txType === TransactionType.ETHSend) {
      const sendAmount = formatBalance(value, selectedNetwork.decimals)
      const sendFiatAmount = formatFiatBalance(value, selectedNetwork.decimals, selectedNetworkPrice)
      const gasAmount = formatBalance(gasPrice, selectedNetwork.decimals)
      const gasFiatAmount = formatFiatBalance(gasPrice, selectedNetwork.decimals, selectedNetworkPrice)
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
      const ERC20TokenPrice = getTokenPrice(ERC20Token?.symbol ?? '').price
      const sendAmount = formatBalance(txArgs[1], ERC20Token?.decimals ?? 18)
      const sendFiatAmount = formatFiatBalance(txArgs[1], ERC20Token?.decimals ?? 18, ERC20TokenPrice)
      const gasAmount = formatBalance(gasPrice, selectedNetwork.decimals)
      const gasFiatAmount = formatFiatBalance(gasPrice, selectedNetwork.decimals, selectedNetworkPrice)
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
    } else {
      const sendAmount = formatBalance(value, selectedNetwork.decimals)
      const sendFiatAmount = formatFiatBalance(value, selectedNetwork.decimals, selectedNetworkPrice)
      const gasAmount = formatBalance(gasPrice, selectedNetwork.decimals)
      const gasFiatAmount = formatFiatBalance(gasPrice, selectedNetwork.decimals, selectedNetworkPrice)
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
  }, [transactionInfo])

  const onSelectTab = (tab: confirmPanelTabs) => () => {
    setSelectedTab(tab)
  }

  const findAccountName = (address: string) => {
    return accounts.find((account) => account.address.toLowerCase() === address.toLowerCase())?.name
  }

  return (
    <StyledWrapper>
      <TopRow>
        <NetworkText>{reduceNetworkDisplayName(selectedNetwork.chainName)}</NetworkText>
      </TopRow>
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
      <MessageBox isDetails={selectedTab === 'details'}>
        {selectedTab === 'transaction' ? (
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
        ) : (
          <TransactionDetailBox hasNoData={transaction.hasNoData} transactionData={transactionInfo.txParams} />
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
