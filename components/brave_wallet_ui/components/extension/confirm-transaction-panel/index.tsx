import * as React from 'react'
import { create } from 'ethereum-blockies'
import { WalletAccountType, Network, TransactionPanelPayload } from '../../../constants/types'
import { reduceAddress } from '../../../utils/reduce-address'
import { NetworkOptions } from '../../../options/network-options'
import locale from '../../../constants/locale'
import { formatBalance, formatFiatBalance } from '../../../utils/format-balances'
import { NavButton } from '../'

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
  MoreButton,
  AccountCircleWrapper,
  ArrowIcon,
  FromToRow,
  Divider,
  SectionRow,
  SectionRightColumn,
  EditButton
} from './style'

export interface Props {
  selectedAccount: WalletAccountType
  transactionPayload: TransactionPanelPayload
  selectedNetwork: Network
  onConfirm: () => void
  onReject: () => void
  onClickMore: () => void
}

function ConfirmTransactionPanel (props: Props) {
  const {
    selectedAccount,
    selectedNetwork,
    transactionPayload,
    onConfirm,
    onReject,
    onClickMore
  } = props

  const fromOrb = React.useMemo(() => {
    return create({ seed: selectedAccount.address, size: 8, scale: 16 }).toDataURL()
  }, [selectedAccount.address])

  const toOrb = React.useMemo(() => {
    return create({ seed: transactionPayload.toAddress, size: 8, scale: 10 }).toDataURL()
  }, [transactionPayload])

  const formatedAmounts = React.useMemo(() => {
    const sendAmount = formatBalance(transactionPayload.transactionAmount, transactionPayload.erc20Token.decimals)
    const sendFiatAmount = formatFiatBalance(transactionPayload.transactionAmount, transactionPayload.erc20Token.decimals, transactionPayload.tokenPrice)
    const gasAmount = formatBalance(transactionPayload.transactionGas, 18)
    const gasFiatAmount = formatFiatBalance(transactionPayload.transactionGas, 18, transactionPayload.ethPrice)
    const grandTotalFiatAmount = Number(sendFiatAmount) + Number(gasFiatAmount)
    return {
      sendAmount,
      sendFiatAmount,
      gasAmount,
      gasFiatAmount,
      grandTotalFiatAmount
    }
  }, [transactionPayload])

  return (
    <StyledWrapper>
      <TopRow>
        <NetworkText>{NetworkOptions[selectedNetwork].abbr}</NetworkText>
        <MoreButton onClick={onClickMore} />
      </TopRow>
      <AccountCircleWrapper>
        <FromCircle orb={fromOrb} />
        <ToCircle orb={toOrb} />
      </AccountCircleWrapper>
      <FromToRow>
        <AccountNameText>{selectedAccount.name}</AccountNameText>
        <ArrowIcon />
        <AccountNameText>{reduceAddress(transactionPayload.toAddress)}</AccountNameText>
      </FromToRow>
      <TransactionTypeText>{locale.confrimTransactionBid}</TransactionTypeText>
      <TransactionAmmountBig>{formatedAmounts.sendAmount} {transactionPayload.erc20Token.symbol}</TransactionAmmountBig>
      <TransactionFiatAmountBig>${formatedAmounts.sendFiatAmount}</TransactionFiatAmountBig>
      <MessageBox>
        <SectionRow>
          <TransactionTitle>{locale.confirmTransactionGasFee}</TransactionTitle>
          <SectionRightColumn>
            <EditButton>{locale.allowSpendEditButton}</EditButton>
            <TransactionTypeText>{formatedAmounts.gasAmount} ETH</TransactionTypeText>
            <TransactionText>${formatedAmounts.gasFiatAmount}</TransactionText>
          </SectionRightColumn>
        </SectionRow>
        <Divider />
        <SectionRow>
          <TransactionTitle>{locale.confirmTransactionTotal}</TransactionTitle>
          <SectionRightColumn>
            <TransactionText>{locale.confirmTransactionAmountGas}</TransactionText>
            <GrandTotalText>{formatedAmounts.sendAmount} {transactionPayload.erc20Token.symbol} + {formatedAmounts.gasAmount} ETH</GrandTotalText>
            <TransactionText>${formatedAmounts.grandTotalFiatAmount}</TransactionText>
          </SectionRightColumn>
        </SectionRow>
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
