import * as React from 'react'
import { create } from 'ethereum-blockies'
import { WalletAccountType, EthereumChain, TransactionInfo } from '../../../constants/types'
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
  transactionInfo: TransactionInfo
  selectedNetwork: EthereumChain
  ethPrice: string
  onConfirm: () => void
  onReject: () => void
}

function ConfirmTransactionPanel (props: Props) {
  const {
    accounts,
    selectedNetwork,
    transactionInfo,
    ethPrice,
    onConfirm,
    onReject
  } = props
  const [selectedTab, setSelectedTab] = React.useState<confirmPanelTabs>('transaction')
  const fromOrb = React.useMemo(() => {
    return create({ seed: transactionInfo.fromAddress, size: 8, scale: 16 }).toDataURL()
  }, [transactionInfo])

  const toOrb = React.useMemo(() => {
    return create({ seed: transactionInfo.txData.baseData.to, size: 8, scale: 10 }).toDataURL()
  }, [transactionInfo])

  const formatedTransaction = React.useMemo(() => {
    const { baseData } = transactionInfo.txData
    const { gasPrice, value, data } = baseData
    if (data.length === 0) {
      const sendAmount = formatBalance(value, 18)
      const sendFiatAmount = formatFiatBalance(value, 18, ethPrice)
      const gasAmount = formatBalance(gasPrice, 18)
      const gasFiatAmount = formatFiatBalance(gasPrice, 18, ethPrice)
      const grandTotalFiatAmount = Number(sendFiatAmount) + Number(gasFiatAmount)
      return {
        sendAmount,
        sendFiatAmount,
        gasAmount,
        gasFiatAmount,
        grandTotalFiatAmount,
        symbol: 'ETH'
      }
    } else {
      return
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
        <AccountNameText>{reduceAddress(transactionInfo.txData.baseData.to)}</AccountNameText>
      </FromToRow>
      <TransactionTypeText>{locale.confrimTransactionBid}</TransactionTypeText>
      <TransactionAmmountBig>{formatedTransaction?.sendAmount} {formatedTransaction?.symbol}</TransactionAmmountBig>
      <TransactionFiatAmountBig>${formatedTransaction?.sendFiatAmount}</TransactionFiatAmountBig>
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
                <TransactionTypeText>{formatedTransaction?.gasAmount} {selectedNetwork.symbol}</TransactionTypeText>
                <TransactionText>${formatedTransaction?.gasFiatAmount}</TransactionText>
              </SectionRightColumn>
            </SectionRow>
            <Divider />
            <SectionRow>
              <TransactionTitle>{locale.confirmTransactionTotal}</TransactionTitle>
              <SectionRightColumn>
                <TransactionText>{locale.confirmTransactionAmountGas}</TransactionText>
                <GrandTotalText>{formatedTransaction?.sendAmount} {formatedTransaction?.symbol} + {formatedTransaction?.gasAmount} {selectedNetwork.symbol}</GrandTotalText>
                <TransactionText>${formatedTransaction?.grandTotalFiatAmount}</TransactionText>
              </SectionRightColumn>
            </SectionRow>
          </>
        ) : (
          <TransactionDetailBox transactionData={transactionInfo.txData.baseData.data} />
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
