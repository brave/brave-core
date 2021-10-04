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
import { reduceAccountDisplayName } from '../../../utils/reduce-account-name'
import { getLocale } from '../../../../common/locale'
import { usePricing, useTransactionParser } from '../../../common/hooks'

import { NavButton, PanelTab, TransactionDetailBox, EditGas } from '../'

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
    selectedNetwork,
    transactionInfo,
    visibleTokens,
    transactionSpotPrices,
    onConfirm,
    onReject
  } = props

  const [selectedTab, setSelectedTab] = React.useState<confirmPanelTabs>('transaction')
  const [isEditing, setIsEditing] = React.useState<boolean>(false)

  // Will remove this hardcoded value once we know
  // where the site info will be coming from.
  const siteURL = 'https://app.compound.finance'

  const findSpotPrice = usePricing(transactionSpotPrices)
  const parseTransaction = useTransactionParser(selectedNetwork, transactionSpotPrices, visibleTokens)
  const transactionDetails = parseTransaction(transactionInfo)

  const onSelectTab = (tab: confirmPanelTabs) => () => {
    setSelectedTab(tab)
  }

  const findAccountName = (address: string) => {
    return accounts.find((account) => account.address.toLowerCase() === address.toLowerCase())?.name
  }

  const fromOrb = React.useMemo(() => {
    return create({ seed: transactionInfo.fromAddress.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [transactionInfo])

  const toOrb = React.useMemo(() => {
    return create({ seed: transactionDetails.sendTo.toLowerCase(), size: 8, scale: 10 }).toDataURL()
  }, [transactionInfo])

  const onToggleEditGas = () => {
    setIsEditing(!isEditing)
  }

  const onSaveGasPrice = () => {
    // Logic here to save gas prices
  }

  return (
    <>
      {isEditing ? (
        <EditGas
          transactionInfo={transactionInfo}
          onCancel={onToggleEditGas}
          networkSpotPrice={findSpotPrice(selectedNetwork.symbol)}
          selectedNetwork={selectedNetwork}
          onSave={onSaveGasPrice}
        />
      ) : (
        <StyledWrapper>
          <TopRow>
            <NetworkText>{reduceNetworkDisplayName(selectedNetwork.chainName)}</NetworkText>
            {transactionInfo.txType === TransactionType.ERC20Approve &&
              <AddressAndOrb>
                <AddressText>{reduceAddress(transactionDetails.sendTo)}</AddressText>
                <AccountCircle orb={toOrb} />
              </AddressAndOrb>
            }
          </TopRow>
          {transactionInfo.txType === TransactionType.ERC20Approve ? (
            <>
              <FavIcon src={`chrome://favicon/size/64@1x/${siteURL}`} />
              <URLText>{siteURL}</URLText>
              <PanelTitle>{getLocale('braveWalletAllowSpendTitle')} {transactionDetails.symbol}?</PanelTitle>
              {/* Will need to allow parameterized locales by introducing the "t" helper. For ex: {t(locale.allowSpendDescription, [spendPayload.erc20Token.symbol])}*/}
              <Description>{getLocale('braveWalletAllowSpendDescriptionFirstHalf')}{transactionDetails.symbol}{getLocale('braveWalletAllowSpendDescriptionSecondHalf')}</Description>
            </>
          ) : (
            <>
              <AccountCircleWrapper>
                <FromCircle orb={fromOrb} />
                <ToCircle orb={toOrb} />
              </AccountCircleWrapper>
              <FromToRow>
                <AccountNameText>{reduceAccountDisplayName(findAccountName(transactionInfo.fromAddress) ?? '', 11)}</AccountNameText>
                <ArrowIcon />
                <AccountNameText>{reduceAddress(transactionDetails.sendTo)}</AccountNameText>
              </FromToRow>
              <TransactionTypeText>{getLocale('braveWalletSend')}</TransactionTypeText>
              <TransactionAmmountBig>{transactionDetails.sendAmount} {transactionDetails.symbol}</TransactionAmmountBig>
              <TransactionFiatAmountBig>${transactionDetails.sendAmountFiat}</TransactionFiatAmountBig>
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
          <MessageBox isDetails={selectedTab === 'details'} isApprove={transactionInfo.txType === TransactionType.ERC20Approve}>
            {selectedTab === 'transaction' ? (
              <>
                {transactionInfo.txType === TransactionType.ERC20Approve &&
                  <>
                    <MessageBoxRow>
                      <TransactionTitle>{getLocale('braveWalletAllowSpendTransactionFee')}</TransactionTitle>
                      <EditButton onClick={onToggleEditGas}>{getLocale('braveWalletAllowSpendEditButton')}</EditButton>
                    </MessageBoxRow>
                    <FiatRow>
                      <TransactionTypeText>{transactionDetails.gasFee} {selectedNetwork.symbol}</TransactionTypeText>
                    </FiatRow>
                    <FiatRow>
                      <TransactionText>${transactionDetails.gasFeeFiat}</TransactionText>
                    </FiatRow>
                  </>
                }
                {transactionInfo.txType !== TransactionType.ERC20Approve &&
                  <>
                    <SectionRow>
                      <TransactionTitle>{getLocale('braveWalletConfirmTransactionGasFee')}</TransactionTitle>
                      <SectionRightColumn>
                        {/* Disabled until wired up to the API*/}
                        <TransactionTypeText>{transactionDetails.gasFee} {selectedNetwork.symbol}</TransactionTypeText>
                        <TransactionText>${transactionDetails.gasFeeFiat}</TransactionText>
                      </SectionRightColumn>
                    </SectionRow>
                    <Divider />
                    <SectionRow>
                      <TransactionTitle>{getLocale('braveWalletConfirmTransactionTotal')}</TransactionTitle>
                      <SectionRightColumn>
                        <TransactionText>{getLocale('braveWalletConfirmTransactionAmountGas')}</TransactionText>
                        <GrandTotalText>{transactionDetails.sendAmount} {transactionDetails.symbol} + {transactionDetails.gasFee} {selectedNetwork.symbol}</GrandTotalText>
                        <TransactionText>${transactionDetails.totalAmountFiat}</TransactionText>
                      </SectionRightColumn>
                    </SectionRow>
                  </>
                }
              </>
            ) : <TransactionDetailBox transactionInfo={transactionInfo}/>}
          </MessageBox>
          <ButtonRow>
            <NavButton
              buttonType='reject'
              text={getLocale('braveWalletAllowSpendRejectButton')}
              onSubmit={onReject}
            />
            <NavButton
              buttonType='confirm'
              text={getLocale('braveWalletAllowSpendConfirmButton')}
              onSubmit={onConfirm}
            />
          </ButtonRow>
        </StyledWrapper>
      )}
    </>

  )
}

export default ConfirmTransactionPanel
