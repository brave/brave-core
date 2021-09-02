import * as React from 'react'
import { create } from 'ethereum-blockies'
import { AllowSpendReturnPayload, EthereumChain, TransactionInfo, TransactionType } from '../../../constants/types'
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

  const transactionInfo: TransactionInfo = {
    fromAddress: '0x7d66c9ddAED3115d93Bd1790332f3Cd06Cf52B14',
    id: '465a4d6646-kjlwf665',
    txArgs: [''],
    txData: {
      baseData: {
        nonce: '0x1',
        gasPrice: '7548000000000000',
        gasLimit: '7548000000000000',
        to: '0x0d8775f648430679a709e98d2b0cb6250d2887ef',
        value: '0x15ddf09c97b0000',
        data: new Uint8Array(24)
      },
      chainId: '0x0',
      maxPriorityFeePerGas: '',
      maxFeePerGas: ''
    },
    txHash: '0xab834bab0000000000000000000000007be8076f4ea4a4ad08075c2508e481d6c946d12b00000000000000000000000073a29a1da971497',
    txStatus: 0,
    txParams: ['Parameters: [ {"type": "uint256"}, {"type": "address[]"}, {"type": "address"}, {"type": "uint256"} ]',
      '0xab834bab0000000000000000000000007be8076f4ea4a4ad08075c2508e481d6c946d12b00000000000000000000000073a29a1da97149722eb09c526e4ead698895bdc'],
    txType: TransactionType.ETHSend
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
            <TransactionDetailBox hasNoData={true} transactionInfo={transactionInfo} />
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
