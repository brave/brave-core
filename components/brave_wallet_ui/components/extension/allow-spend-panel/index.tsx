import * as React from 'react'
import { create } from 'ethereum-blockies'
import { AllowSpendReturnPayload, EthereumChain } from '../../../constants/types'
import { reduceAddress } from '../../../utils/reduce-address'
import locale from '../../../constants/locale'
import { NavButton, PanelTab, TransactionDetailBox } from '../'

// Styled Components
import {
  MessageBox,
  TransactionText,
  MessageBoxRow,
  EditButton,
  ButtonRow,
  FavIcon,
  URLText,
  BalanceText,
  FiatRow,
  FiatBalanceText
} from './style'

import {
  StyledWrapper,
  AccountCircle,
  AddressAndOrb,
  AddressText,
  CenterColumn,
  Description,
  NetworkText,
  PanelTitle,
  TopRow,
  TabRow
} from '../shared-panel-styles'

export type allowSpendPanelTabs = 'transaction' | 'details'

export interface Props {
  selectedNetwork: EthereumChain
  onConfirm: () => void
  onReject: () => void
  spendPayload: AllowSpendReturnPayload
}

function AllowSpendPanel (props: Props) {
  const {
    selectedNetwork,
    spendPayload,
    onConfirm,
    onReject
  } = props
  const [selectedTab, setSelectedTab] = React.useState<allowSpendPanelTabs>('transaction')
  const orb = React.useMemo(() => {
    return create({ seed: spendPayload.contractAddress, size: 8, scale: 16 }).toDataURL()
  }, [spendPayload.contractAddress])

  const onSelectTab = (tab: allowSpendPanelTabs) => () => {
    setSelectedTab(tab)
  }

  return (
    <StyledWrapper>
      <TopRow>
        <NetworkText>{selectedNetwork.chainName}</NetworkText>
        <AddressAndOrb>
          <AddressText>{reduceAddress(spendPayload.contractAddress)}</AddressText>
          <AccountCircle orb={orb} />
        </AddressAndOrb>
      </TopRow>
      <CenterColumn>
        <FavIcon src={`chrome://favicon/size/64@1x/${spendPayload.siteUrl}`} />
        <URLText>{spendPayload.siteUrl}</URLText>
        <PanelTitle>{locale.allowSpendTitle} {spendPayload.erc20Token.symbol}?</PanelTitle>
        {/* Will need to allow parameterized locales by introducing the "t" helper. For ex: {t(locale.allowSpendDescription, [spendPayload.erc20Token.symbol])}*/}
        <Description>{locale.allowSpendDescriptionFirstHalf}{spendPayload.erc20Token.symbol}{locale.allowSpendDescriptionSecondHalf}</Description>
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
        <MessageBox>
          {selectedTab === 'transaction' ? (
            <>
              <MessageBoxRow>
                <TransactionText>{locale.allowSpendTransactionFee}</TransactionText>
                <EditButton>{locale.allowSpendEditButton}</EditButton>
              </MessageBoxRow>
              <FiatRow>
                <BalanceText>{spendPayload.transactionFeeWei} ETH</BalanceText>
              </FiatRow>
              <FiatRow>
                <FiatBalanceText>${spendPayload.transactionFeeWei}</FiatBalanceText>
              </FiatRow>
            </>
          ) : (
            <TransactionDetailBox transactionData={spendPayload.transactionData} />
          )}
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
