import * as React from 'react'
import { create } from 'ethereum-blockies'
import { WalletAccountType, Network, AllowSpendReturnPayload } from '../../../constants/types'
import { reduceAddress } from '../../../utils/reduce-address'
import { NetworkOptions } from '../../../options/network-options'
import locale from '../../../constants/locale'
import { NavButton } from '../'
// Styled Components
import {
  StyledWrapper,
  CenterColumn,
  AccountCircle,
  TopRow,
  NetworkText,
  PanelTitle,
  Description,
  MessageBox,
  MessageBoxTitle,
  TransactionText,
  MessageBoxRow,
  EditButton,
  DetailsButton,
  AddressAndOrb,
  AddressText,
  ButtonRow,
  FavIcon,
  URLText,
  BalanceText,
  FiatRow,
  FiatBalanceText
} from './style'

export interface Props {
  selectedAccount: WalletAccountType
  selectedNetwork: Network
  onConfirm: () => void
  onReject: () => void
  spendPayload: AllowSpendReturnPayload
}

function AllowSpendPanel (props: Props) {
  const {
    selectedAccount,
    selectedNetwork,
    spendPayload,
    onConfirm,
    onReject
  } = props

  const orb = React.useMemo(() => {
    return create({ seed: selectedAccount.address, size: 8, scale: 16 }).toDataURL()
  }, [selectedAccount.address])

  return (
    <StyledWrapper>
      <TopRow>
        <NetworkText>{NetworkOptions[selectedNetwork].abbr}</NetworkText>
        <AddressAndOrb>
          <AddressText>{reduceAddress(selectedAccount.address)}</AddressText>
          <AccountCircle orb={orb} />
        </AddressAndOrb>
      </TopRow>
      <CenterColumn>
        <FavIcon src={spendPayload.sitFavIcon} />
        <URLText>{spendPayload.siteUrl}</URLText>
        <PanelTitle>{locale.allowSpendTitle} {spendPayload.erc20Token.symbol}?</PanelTitle>
        <Description>{locale.allowSpendDescriptionFirstHalf}{spendPayload.erc20Token.symbol}{locale.allowSpendDescriptionSecondHalf}</Description>
        <MessageBoxTitle>{locale.allowSpendBoxTitle}</MessageBoxTitle>
        <MessageBox>
          <MessageBoxRow>
            <TransactionText>{locale.allowSpendTransactionFee}</TransactionText>
            <EditButton>{locale.allowSpendEditButton}</EditButton>
          </MessageBoxRow>
          <MessageBoxRow>
            <DetailsButton>{locale.allowSpendDetailsButton}</DetailsButton>
            <BalanceText>{spendPayload.transactionFeeWei} ETH</BalanceText>
          </MessageBoxRow>
          <FiatRow>
            <FiatBalanceText>${spendPayload.transactionFeeWei}</FiatBalanceText>
          </FiatRow>
        </MessageBox>
      </CenterColumn>
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

export default AllowSpendPanel
