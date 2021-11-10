import * as React from 'react'
import { create } from 'ethereum-blockies'

import {
  WalletAccountType,
  EthereumChain,
  TransactionInfo,
  TransactionType,
  AssetPrice,
  ERCToken,
  GasEstimation1559
} from '../../../constants/types'
import {
  UpdateUnapprovedTransactionGasFieldsType,
  UpdateUnapprovedTransactionSpendAllowanceType
} from '../../../common/constants/action_types'
import { reduceAddress } from '../../../utils/reduce-address'
import { reduceNetworkDisplayName } from '../../../utils/network-utils'
import { reduceAccountDisplayName } from '../../../utils/reduce-account-name'
import { formatBalance, toWeiHex } from '../../../utils/format-balances'
import { getLocale } from '../../../../common/locale'
import { usePricing, useTransactionParser } from '../../../common/hooks'
import { withPlaceholderIcon } from '../../shared'

import { NavButton, PanelTab, TransactionDetailBox } from '../'
import EditGas, { MaxPriorityPanels } from '../edit-gas'
import EditAllowance from '../edit-allowance'

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
  FavIcon,
  URLText,
  QueueStepText,
  QueueStepRow,
  QueueStepButton,
  TopColumn,
  AssetIcon,
  ErrorText
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
  visibleTokens: ERCToken[]
  fullTokenList: ERCToken[]
  transactionInfo: TransactionInfo
  selectedNetwork: EthereumChain
  transactionSpotPrices: AssetPrice[]
  gasEstimates?: GasEstimation1559
  transactionsQueueLength: number
  transactionQueueNumber: number
  onQueueNextTransction: () => void
  onConfirm: () => void
  onReject: () => void
  onRejectAllTransactions: () => void
  refreshGasEstimates: () => void
  getERC20Allowance: (recipient: string, sender: string, approvalTarget: string) => Promise<string>
  updateUnapprovedTransactionGasFields: (payload: UpdateUnapprovedTransactionGasFieldsType) => void
  updateUnapprovedTransactionSpendAllowance: (payload: UpdateUnapprovedTransactionSpendAllowanceType) => void
}

function ConfirmTransactionPanel (props: Props) {
  const {
    accounts,
    selectedNetwork,
    transactionInfo,
    visibleTokens,
    transactionSpotPrices,
    gasEstimates,
    transactionsQueueLength,
    transactionQueueNumber,
    fullTokenList,
    onQueueNextTransction,
    onConfirm,
    onReject,
    onRejectAllTransactions,
    refreshGasEstimates,
    getERC20Allowance,
    updateUnapprovedTransactionGasFields,
    updateUnapprovedTransactionSpendAllowance
  } = props

  const { txData: { gasEstimation: transactionGasEstimates } } = transactionInfo

  const [maxPriorityPanel, setMaxPriorityPanel] = React.useState<MaxPriorityPanels>(MaxPriorityPanels.setSuggested)
  const [suggestedSliderStep, setSuggestedSliderStep] = React.useState<string>('1')
  const [suggestedMaxPriorityFeeChoices, setSuggestedMaxPriorityFeeChoices] = React.useState<string[]>([
    transactionGasEstimates?.slowMaxPriorityFeePerGas || '0',
    transactionGasEstimates?.avgMaxPriorityFeePerGas || '0',
    transactionGasEstimates?.fastMaxPriorityFeePerGas || '0'
  ])
  const [baseFeePerGas, setBaseFeePerGas] = React.useState<string>(transactionGasEstimates?.baseFeePerGas || '')
  const [selectedTab, setSelectedTab] = React.useState<confirmPanelTabs>('transaction')
  const [isEditing, setIsEditing] = React.useState<boolean>(false)
  const [currentTokenAllowance, setCurrentTokenAllowance] = React.useState<string>('')
  const [isEditingAllowance, setIsEditingAllowance] = React.useState<boolean>(false)

  // Will remove this hardcoded value once we know
  // where the site info will be coming from.
  const siteURL = 'https://app.compound.finance'

  const findSpotPrice = usePricing(transactionSpotPrices)
  const parseTransaction = useTransactionParser(selectedNetwork, accounts, transactionSpotPrices, visibleTokens, fullTokenList)
  const transactionDetails = parseTransaction(transactionInfo)

  React.useEffect(() => {
    const interval = setInterval(() => {
      refreshGasEstimates()
    }, 15000)

    refreshGasEstimates()
    return () => clearInterval(interval)
  }, [])

  React.useEffect(
    () => {
      setSuggestedMaxPriorityFeeChoices([
        gasEstimates?.slowMaxPriorityFeePerGas || '0',
        gasEstimates?.avgMaxPriorityFeePerGas || '0',
        gasEstimates?.fastMaxPriorityFeePerGas || '0'
      ])

      setBaseFeePerGas(gasEstimates?.baseFeePerGas || '0')
    },
    [gasEstimates]
  )

  React.useEffect(() => {
    if (transactionInfo.txType !== TransactionType.ERC20Approve) {
      return
    }

    if (!transactionDetails.approvalTarget) {
      return
    }

    getERC20Allowance(
      transactionDetails.recipient,
      transactionDetails.sender,
      transactionDetails.approvalTarget
    ).then(result => {
      const allowance = formatBalance(result, transactionDetails.decimals)
      setCurrentTokenAllowance(allowance)
    }).catch(e => console.error(e))
  }, [])

  const onSelectTab = (tab: confirmPanelTabs) => () => {
    setSelectedTab(tab)
  }

  const findAccountName = (address: string) => {
    return accounts.find((account) => account.address.toLowerCase() === address.toLowerCase())?.name
  }

  const fromOrb = React.useMemo(() => {
    return create({ seed: transactionDetails.sender.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [transactionDetails])

  const toOrb = React.useMemo(() => {
    return create({ seed: transactionDetails.recipient.toLowerCase(), size: 8, scale: 10 }).toDataURL()
  }, [transactionDetails])

  const onToggleEditGas = () => {
    setIsEditing(!isEditing)
  }

  const onToggleEditAllowance = () => {
    setIsEditingAllowance(!isEditingAllowance)
  }

  const onEditAllowanceSave = (allowance: string) => {
    updateUnapprovedTransactionSpendAllowance({
      txMetaId: transactionInfo.id,
      spenderAddress: transactionDetails.approvalTarget || '',
      allowance: toWeiHex(allowance, transactionDetails.decimals)
    })
  }

  const AssetIconWithPlaceholder = React.useMemo(() => {
    return withPlaceholderIcon(AssetIcon, { size: 'big', marginLeft: 0, marginRight: 0 })
  }, [])

  const transactionTitle = React.useMemo(
    (): string =>
      transactionDetails.isSwap
        ? getLocale('braveWalletSwap')
        : getLocale('braveWalletSend')
    , [transactionDetails])

  if (isEditing) {
    return (
      <EditGas
        transactionInfo={transactionInfo}
        onCancel={onToggleEditGas}
        networkSpotPrice={findSpotPrice(selectedNetwork.symbol)}
        selectedNetwork={selectedNetwork}
        baseFeePerGas={baseFeePerGas}
        suggestedMaxPriorityFeeChoices={suggestedMaxPriorityFeeChoices}
        updateUnapprovedTransactionGasFields={updateUnapprovedTransactionGasFields}
        suggestedSliderStep={suggestedSliderStep}
        setSuggestedSliderStep={setSuggestedSliderStep}
        maxPriorityPanel={maxPriorityPanel}
        setMaxPriorityPanel={setMaxPriorityPanel}
      />
    )
  }

  if (isEditingAllowance) {
    return (
      <EditAllowance
        onCancel={onToggleEditAllowance}
        onSave={onEditAllowanceSave}
        proposedAllowance={transactionDetails.value}
        symbol={transactionDetails.symbol}
        approvalTarget={transactionDetails.approvalTargetLabel || ''}
      />
    )
  }

  return (
    <StyledWrapper>
      <TopRow>
        <NetworkText>{reduceNetworkDisplayName(selectedNetwork.chainName)}</NetworkText>
        {transactionInfo.txType === TransactionType.ERC20Approve &&
          <AddressAndOrb>
            <AddressText>{reduceAddress(transactionDetails.recipient)}</AddressText>
            <AccountCircle orb={toOrb} />
          </AddressAndOrb>
        }
        {transactionsQueueLength > 1 &&
          <QueueStepRow>
            <QueueStepText>{transactionQueueNumber} {getLocale('braveWalletQueueOf')} {transactionsQueueLength}</QueueStepText>
            <QueueStepButton
              onClick={onQueueNextTransction}
            >
              {transactionQueueNumber === transactionsQueueLength
                ? getLocale('braveWalletQueueFirst')
                : getLocale('braveWalletQueueNext')
              }
            </QueueStepButton>
          </QueueStepRow>
        }
      </TopRow>
      {transactionInfo.txType === TransactionType.ERC20Approve ? (
        <>
          <FavIcon src={`chrome://favicon/size/64@1x/${siteURL}`} />
          <URLText>{siteURL}</URLText>
          <PanelTitle>{getLocale('braveWalletAllowSpendTitle').replace('$1', transactionDetails.symbol)}</PanelTitle>
          <Description>{getLocale('braveWalletAllowSpendDescription').replace('$1', transactionDetails.symbol)}</Description>
          <EditButton onClick={onToggleEditAllowance}>{getLocale('braveWalletEditPermissionsButton')}</EditButton>
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
            <AccountNameText>{reduceAddress(transactionDetails.recipient)}</AccountNameText>
          </FromToRow>
          <TransactionTypeText>{transactionTitle}</TransactionTypeText>
          {(transactionInfo.txType === TransactionType.ERC721TransferFrom ||
            transactionInfo.txType === TransactionType.ERC721SafeTransferFrom) &&
            <AssetIconWithPlaceholder selectedAsset={transactionDetails.erc721ERCToken} />
          }
          <TransactionAmmountBig>
            {transactionInfo.txType === TransactionType.ERC721TransferFrom ||
              transactionInfo.txType === TransactionType.ERC721SafeTransferFrom
              ? transactionDetails.erc721ERCToken?.name + ' ' + transactionDetails.erc721TokenId
              : transactionDetails.value + ' ' + transactionDetails.symbol
            }
          </TransactionAmmountBig>
          {transactionInfo.txType !== TransactionType.ERC721TransferFrom &&
            transactionInfo.txType !== TransactionType.ERC721SafeTransferFrom &&
            <TransactionFiatAmountBig>${transactionDetails.fiatValue}</TransactionFiatAmountBig>
          }
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
                <TopColumn>
                  <EditButton onClick={onToggleEditGas}>{getLocale('braveWalletAllowSpendEditButton')}</EditButton>
                  <SectionRow>
                    <TransactionTitle>{getLocale('braveWalletAllowSpendTransactionFee')}</TransactionTitle>
                    <TransactionTypeText>{transactionDetails.gasFee} {selectedNetwork.symbol}</TransactionTypeText>
                  </SectionRow>
                  <TransactionText
                    hasError={transactionDetails.insufficientFundsError}
                  >
                    {transactionDetails.insufficientFundsError ? `${getLocale('braveWalletSwapInsufficientBalance')} ` : ''}
                    ${transactionDetails.gasFeeFiat}
                  </TransactionText>
                </TopColumn>
                <Divider />
                <SectionRow>
                  <TransactionTitle>{getLocale('braveWalletAllowSpendCurrentAllowance')}</TransactionTitle>
                  <SectionRightColumn>
                    <TransactionTypeText>{currentTokenAllowance} {transactionDetails.symbol}</TransactionTypeText>
                    <TransactionText />
                  </SectionRightColumn>
                </SectionRow>
                <Divider />
                <SectionRow>
                  <TransactionTitle>{getLocale('braveWalletAllowSpendProposedAllowance')}</TransactionTitle>
                  <SectionRightColumn>
                    <TransactionTypeText>{transactionDetails.value} {transactionDetails.symbol}</TransactionTypeText>
                    <TransactionText />
                  </SectionRightColumn>
                </SectionRow>
              </>
            }
            {transactionInfo.txType !== TransactionType.ERC20Approve &&
              <>
                <SectionRow>
                  <TransactionTitle>{getLocale('braveWalletConfirmTransactionGasFee')}</TransactionTitle>
                  <SectionRightColumn>
                    <EditButton onClick={onToggleEditGas}>{getLocale('braveWalletAllowSpendEditButton')}</EditButton>
                    <TransactionTypeText>{transactionDetails.gasFee} {selectedNetwork.symbol}</TransactionTypeText>
                    <TransactionText>${transactionDetails.gasFeeFiat}</TransactionText>
                  </SectionRightColumn>
                </SectionRow>
                <Divider />
                <SectionRow>
                  <TransactionTitle>{getLocale('braveWalletConfirmTransactionTotal')}</TransactionTitle>
                  <SectionRightColumn>
                    <TransactionText>{getLocale('braveWalletConfirmTransactionAmountGas')}</TransactionText>
                    <GrandTotalText>{transactionDetails.value} {transactionDetails.symbol} + {transactionDetails.gasFee} {selectedNetwork.symbol}</GrandTotalText>
                    <TransactionText
                      hasError={transactionDetails.insufficientFundsError}
                    >
                      {transactionDetails.insufficientFundsError ? `${getLocale('braveWalletSwapInsufficientBalance')} ` : ''}
                      ${transactionDetails.fiatTotal}
                    </TransactionText>
                  </SectionRightColumn>
                </SectionRow>
              </>
            }
          </>
        ) : <TransactionDetailBox transactionInfo={transactionInfo} />}
      </MessageBox>
      {transactionsQueueLength > 1 &&
        <QueueStepButton
          needsMargin={true}
          onClick={onRejectAllTransactions}
        >
          {getLocale('braveWalletQueueRejectAll').replace('$1', transactionsQueueLength.toString())}
        </QueueStepButton>
      }
      {transactionDetails.addressError &&
        <ErrorText>
          {transactionDetails.addressError}
        </ErrorText>
      }
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
          disabled={transactionDetails.addressError || parseFloat(transactionDetails.gasFeeFiat) === 0 ? true : transactionDetails.insufficientFundsError}
        />
      </ButtonRow>
    </StyledWrapper>
  )
}

export default ConfirmTransactionPanel
