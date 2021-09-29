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
import { formatBalance, formatGasFee, formatFiatGasFee, formatFiatBalance } from '../../../utils/format-balances'
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

  const findTokenInfo = (contractAddress: string) => {
    return visibleTokens.find((account) => account.contractAddress.toLowerCase() === contractAddress.toLowerCase())
  }

  const findSpotPrice = (symbol: string) => {
    return transactionSpotPrices.find((token) => token.fromAsset.toLowerCase() === symbol.toLowerCase())
  }

  // Will remove this hardcoded value once we know
  // where the site info will be coming from.
  const siteURL = 'https://app.compound.finance'

  const getTransactionPriceDisplayInfo = (
    gasPrice: string,
    gasLimit: string,
    network: EthereumChain,
    networkPrice: string,
    sendValue: string,
    sendDecimals: number,
    sendPrice: string
  ) => {
    const sendAmount = formatBalance(sendValue, sendDecimals)
    const sendFiatAmount = formatFiatBalance(sendValue, sendDecimals, sendPrice)
    const gasAmount = formatGasFee(gasPrice, gasLimit, network.decimals)
    const gasFiatAmount = formatFiatGasFee(gasAmount, networkPrice)
    const grandTotalFiatAmount = Number(sendFiatAmount) + Number(gasFiatAmount)
    return {
      sendAmount,
      sendFiatAmount,
      gasAmount,
      gasFiatAmount,
      grandTotalFiatAmount
    }
  }

  const transaction = React.useMemo(() => {
    const { txType, txArgs } = transactionInfo
    const { baseData } = transactionInfo.txData
    const { gasPrice, gasLimit, value, data, to } = baseData
    const networkPrice = findSpotPrice(selectedNetwork.symbol)?.price ?? ''
    const ERC20Token = findTokenInfo(to)
    const ERC20TokenDecimals = ERC20Token?.decimals ?? 18
    const ERC20TokenPrice = findSpotPrice(ERC20Token?.symbol ?? '')?.price ?? ''
    const hasNoData = data.length === 0
    if (txType === TransactionType.ERC20Transfer || txType === TransactionType.ERC20Approve) {
      const priceInfo = getTransactionPriceDisplayInfo(
        gasPrice,
        gasLimit,
        selectedNetwork,
        networkPrice,
        txArgs[1],
        ERC20TokenDecimals,
        ERC20TokenPrice
      )
      return {
        sendTo: txArgs[0],
        symbol: ERC20Token?.symbol ?? '',
        hasNoData,
        ...priceInfo
      }
    } else {
      const priceInfo = getTransactionPriceDisplayInfo(
        gasPrice,
        gasLimit,
        selectedNetwork,
        networkPrice,
        value,
        selectedNetwork.decimals,
        networkPrice
      )
      return {
        sendTo: to,
        symbol: selectedNetwork.symbol,
        hasNoData,
        ...priceInfo
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
    return create({ seed: transactionInfo.fromAddress.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [transactionInfo])

  const toOrb = React.useMemo(() => {
    return create({ seed: transaction.sendTo.toLowerCase(), size: 8, scale: 10 }).toDataURL()
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
          networkSpotPrice={findSpotPrice(selectedNetwork.symbol.toLowerCase())?.price ?? ''}
          selectedNetwork={selectedNetwork}
          onSave={onSaveGasPrice}
        />
      ) : (
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
              <FavIcon src={`chrome://favicon/size/64@1x/${siteURL}`} />
              <URLText>{siteURL}</URLText>
              <PanelTitle>{getLocale('braveWalletAllowSpendTitle')} {transaction.symbol}?</PanelTitle>
              {/* Will need to allow parameterized locales by introducing the "t" helper. For ex: {t(locale.allowSpendDescription, [spendPayload.erc20Token.symbol])}*/}
              <Description>{getLocale('braveWalletAllowSpendDescriptionFirstHalf')}{transaction.symbol}{getLocale('braveWalletAllowSpendDescriptionSecondHalf')}</Description>
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
                <AccountNameText>{reduceAddress(transaction.sendTo)}</AccountNameText>
              </FromToRow>
              <TransactionTypeText>{getLocale('braveWalletSend')}</TransactionTypeText>
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
          <MessageBox isDetails={selectedTab === 'details'} isApprove={transactionInfo.txType === TransactionType.ERC20Approve}>
            {selectedTab === 'transaction' ? (
              <>
                {transactionInfo.txType === TransactionType.ERC20Approve &&
                  <>
                    <MessageBoxRow>
                      <TransactionTitle>{getLocale('braveWalletAllowSpendTransactionFee')}</TransactionTitle>
                      {/* Disabled until wired up to the API*/}
                      <EditButton disabled={true} onClick={onToggleEditGas}>{getLocale('braveWalletAllowSpendEditButton')}</EditButton>
                    </MessageBoxRow>
                    <FiatRow>
                      <TransactionTypeText>{transaction.gasAmount} {selectedNetwork.symbol}</TransactionTypeText>
                    </FiatRow>
                    <FiatRow>
                      <TransactionText>${transaction.gasFiatAmount}</TransactionText>
                    </FiatRow>
                  </>
                }
                {transactionInfo.txType !== TransactionType.ERC20Approve &&
                  <>
                    <SectionRow>
                      <TransactionTitle>{getLocale('braveWalletConfirmTransactionGasFee')}</TransactionTitle>
                      <SectionRightColumn>
                        {/* Disabled until wired up to the API*/}
                        <EditButton disabled={true} onClick={onToggleEditGas}>{getLocale('braveWalletAllowSpendEditButton')}</EditButton>
                        <TransactionTypeText>{transaction.gasAmount} {selectedNetwork.symbol}</TransactionTypeText>
                        <TransactionText>${transaction.gasFiatAmount}</TransactionText>
                      </SectionRightColumn>
                    </SectionRow>
                    <Divider />
                    <SectionRow>
                      <TransactionTitle>{getLocale('braveWalletConfirmTransactionTotal')}</TransactionTitle>
                      <SectionRightColumn>
                        <TransactionText>{getLocale('braveWalletConfirmTransactionAmountGas')}</TransactionText>
                        <GrandTotalText>{transaction.sendAmount} {transaction.symbol} + {transaction.gasAmount} {selectedNetwork.symbol}</GrandTotalText>
                        <TransactionText>${transaction.grandTotalFiatAmount.toFixed(2)}</TransactionText>
                      </SectionRightColumn>
                    </SectionRow>
                  </>
                }
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
