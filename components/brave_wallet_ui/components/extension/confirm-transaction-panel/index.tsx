import * as React from 'react'
import { create } from 'ethereum-blockies'

import {
  BraveWallet,
  WalletAccountType,
  DefaultCurrencies
} from '../../../constants/types'
import {
  UpdateUnapprovedTransactionGasFieldsType,
  UpdateUnapprovedTransactionNonceType,
  UpdateUnapprovedTransactionSpendAllowanceType
} from '../../../common/constants/action_types'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import { reduceNetworkDisplayName } from '../../../utils/network-utils'
import { reduceAccountDisplayName } from '../../../utils/reduce-account-name'
import { formatBalance, toWeiHex } from '../../../utils/format-balances'
import {
  formatWithCommasAndDecimals,
  formatFiatAmountWithCommasAndDecimals,
  formatTokenAmountWithCommasAndDecimals
} from '../../../utils/format-prices'

// Hooks
import { usePricing, useTransactionParser, useTokenInfo } from '../../../common/hooks'

import { getLocale } from '../../../../common/locale'
import { withPlaceholderIcon } from '../../shared'

import { NavButton, PanelTab, TransactionDetailBox } from '../'
import EditGas, { MaxPriorityPanels } from '../edit-gas'
import EditAllowance from '../edit-allowance'

import { getBlockchainTokenInfo } from '../../../common/async/lib'

// Styled Components
import {
  StyledWrapper,
  FromCircle,
  ToCircle,
  AccountNameText,
  TopRow,
  NetworkText,
  TransactionAmountBig,
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
  ErrorText,
  SectionColumn,
  SingleRow
} from './style'

import {
  TabRow,
  Description,
  PanelTitle,
  AccountCircle,
  AddressAndOrb,
  AddressText
} from '../shared-panel-styles'
import AdvancedTransactionSettingsButton from '../advanced-transaction-settings/button'
import AdvancedTransactionSettings from '../advanced-transaction-settings'

export type confirmPanelTabs = 'transaction' | 'details'

export interface Props {
  siteURL: string
  accounts: WalletAccountType[]
  visibleTokens: BraveWallet.BlockchainToken[]
  fullTokenList: BraveWallet.BlockchainToken[]
  transactionInfo: BraveWallet.TransactionInfo
  selectedNetwork: BraveWallet.EthereumChain
  transactionSpotPrices: BraveWallet.AssetPrice[]
  gasEstimates?: BraveWallet.GasEstimation1559
  transactionsQueueLength: number
  transactionQueueNumber: number
  defaultCurrencies: DefaultCurrencies
  onQueueNextTransaction: () => void
  onConfirm: () => void
  onReject: () => void
  onRejectAllTransactions: () => void
  refreshGasEstimates: () => void
  getERC20Allowance: (recipient: string, sender: string, approvalTarget: string) => Promise<string>
  updateUnapprovedTransactionGasFields: (payload: UpdateUnapprovedTransactionGasFieldsType) => void
  updateUnapprovedTransactionSpendAllowance: (payload: UpdateUnapprovedTransactionSpendAllowanceType) => void
  updateUnapprovedTransactionNonce: (payload: UpdateUnapprovedTransactionNonceType) => void
}

function ConfirmTransactionPanel (props: Props) {
  const {
    siteURL,
    accounts,
    selectedNetwork,
    transactionInfo,
    visibleTokens,
    transactionSpotPrices,
    gasEstimates,
    transactionsQueueLength,
    transactionQueueNumber,
    fullTokenList,
    defaultCurrencies,
    onQueueNextTransaction,
    onConfirm,
    onReject,
    onRejectAllTransactions,
    refreshGasEstimates,
    getERC20Allowance,
    updateUnapprovedTransactionGasFields,
    updateUnapprovedTransactionSpendAllowance,
    updateUnapprovedTransactionNonce
  } = props

  const transactionGasEstimates = transactionInfo.txDataUnion.ethTxData1559?.gasEstimation

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
  const [showAdvancedTransactionSettings, setShowAdvancedTransactionSettings] = React.useState<boolean>(false)

  const { findAssetPrice } = usePricing(transactionSpotPrices)
  const parseTransaction = useTransactionParser(selectedNetwork, accounts, transactionSpotPrices, visibleTokens, fullTokenList)
  const transactionDetails = parseTransaction(transactionInfo)

  const {
    onFindTokenInfoByContractAddress,
    foundTokenInfoByContractAddress
  } = useTokenInfo(getBlockchainTokenInfo, visibleTokens, fullTokenList, selectedNetwork)

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
    if (transactionInfo.txType !== BraveWallet.TransactionType.ERC20Approve) {
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

  React.useEffect(() => {
    if (transactionInfo.txType === BraveWallet.TransactionType.ERC20Approve) {
      onFindTokenInfoByContractAddress(transactionDetails.recipient)
    }
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

  const onToggleAdvancedTransactionSettings = () => {
    setShowAdvancedTransactionSettings(!showAdvancedTransactionSettings)
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

  const isConfirmButtonDisabled = React.useMemo(() => {
    return (
      !!transactionDetails.sameAddressError ||
      !!transactionDetails.contractAddressError ||
      transactionDetails.insufficientFundsError ||
      !!transactionDetails.missingGasLimitError
    )
  }, [transactionDetails])

  if (isEditing) {
    return (
      <EditGas
        transactionInfo={transactionInfo}
        onCancel={onToggleEditGas}
        networkSpotPrice={findAssetPrice(selectedNetwork.symbol)}
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

  if (showAdvancedTransactionSettings) {
    return (
      <AdvancedTransactionSettings
        onCancel={onToggleAdvancedTransactionSettings}
        nonce={transactionDetails.nonce}
        txMetaId={transactionInfo.id}
        updateUnapprovedTransactionNonce={updateUnapprovedTransactionNonce}
      />
    )
  }

  return (
    <StyledWrapper>
      <TopRow>
        <NetworkText>{reduceNetworkDisplayName(selectedNetwork.chainName)}</NetworkText>
        {transactionInfo.txType === BraveWallet.TransactionType.ERC20Approve &&
          <AddressAndOrb>
            <AddressText>{reduceAddress(transactionDetails.recipient)}</AddressText>
            <AccountCircle orb={toOrb} />
          </AddressAndOrb>
        }
        {transactionsQueueLength > 1 &&
          <QueueStepRow>
            <QueueStepText>{transactionQueueNumber} {getLocale('braveWalletQueueOf')} {transactionsQueueLength}</QueueStepText>
            <QueueStepButton
              onClick={onQueueNextTransaction}
            >
              {transactionQueueNumber === transactionsQueueLength
                ? getLocale('braveWalletQueueFirst')
                : getLocale('braveWalletQueueNext')
              }
            </QueueStepButton>
          </QueueStepRow>
        }
      </TopRow>
      {transactionInfo.txType === BraveWallet.TransactionType.ERC20Approve ? (
        <>
          <FavIcon src={`chrome://favicon/size/64@1x/${siteURL}`} />
          <URLText>{siteURL}</URLText>
          <PanelTitle>{getLocale('braveWalletAllowSpendTitle').replace('$1', foundTokenInfoByContractAddress?.symbol ?? '')}</PanelTitle>
          <Description>{getLocale('braveWalletAllowSpendDescription').replace('$1', foundTokenInfoByContractAddress?.symbol ?? '')}</Description>
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
          {(transactionInfo.txType === BraveWallet.TransactionType.ERC721TransferFrom ||
            transactionInfo.txType === BraveWallet.TransactionType.ERC721SafeTransferFrom) &&
            <AssetIconWithPlaceholder selectedAsset={transactionDetails.erc721BlockchainToken} />
          }
          <TransactionAmountBig>
            {transactionInfo.txType === BraveWallet.TransactionType.ERC721TransferFrom ||
              transactionInfo.txType === BraveWallet.TransactionType.ERC721SafeTransferFrom
              ? transactionDetails.erc721BlockchainToken?.name + ' ' + transactionDetails.erc721TokenId
              : formatTokenAmountWithCommasAndDecimals(transactionDetails.valueExact, transactionDetails.symbol)
            }
          </TransactionAmountBig>
          {transactionInfo.txType !== BraveWallet.TransactionType.ERC721TransferFrom &&
            transactionInfo.txType !== BraveWallet.TransactionType.ERC721SafeTransferFrom &&
            <TransactionFiatAmountBig>{formatFiatAmountWithCommasAndDecimals(transactionDetails.fiatValue, defaultCurrencies.fiat)}</TransactionFiatAmountBig>
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

        <AdvancedTransactionSettingsButton
          onSubmit={ onToggleAdvancedTransactionSettings }
        />
      </TabRow>
      <MessageBox isDetails={selectedTab === 'details'} isApprove={transactionInfo.txType === BraveWallet.TransactionType.ERC20Approve}>
        {selectedTab === 'transaction' ? (
          <>
            {transactionInfo.txType === BraveWallet.TransactionType.ERC20Approve &&
              <>
                <TopColumn>
                  <EditButton onClick={onToggleEditGas}>{getLocale('braveWalletAllowSpendEditButton')}</EditButton>
                  <SectionRow>
                    <TransactionTitle>{getLocale('braveWalletAllowSpendTransactionFee')}</TransactionTitle>
                    <TransactionTypeText>{formatWithCommasAndDecimals(formatBalance(transactionDetails.gasFee, selectedNetwork.decimals))} {selectedNetwork.symbol}</TransactionTypeText>
                  </SectionRow>
                  <TransactionText
                    hasError={transactionDetails.insufficientFundsError}
                  >
                    {transactionDetails.insufficientFundsError ? `${getLocale('braveWalletSwapInsufficientBalance')} ` : ''}
                    {formatFiatAmountWithCommasAndDecimals(transactionDetails.gasFeeFiat, defaultCurrencies.fiat)}
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
                    <TransactionTypeText>{formatTokenAmountWithCommasAndDecimals(transactionDetails.valueExact, transactionDetails.symbol)}</TransactionTypeText>
                    <TransactionText />
                  </SectionRightColumn>
                </SectionRow>
              </>
            }
            {transactionInfo.txType !== BraveWallet.TransactionType.ERC20Approve &&
              <>
                <SectionRow>
                  <TransactionTitle>{getLocale('braveWalletConfirmTransactionGasFee')}</TransactionTitle>
                  <SectionRightColumn>
                    <EditButton onClick={onToggleEditGas}>{getLocale('braveWalletAllowSpendEditButton')}</EditButton>
                    <TransactionTypeText>{formatWithCommasAndDecimals(formatBalance(transactionDetails.gasFee, selectedNetwork.decimals))} {selectedNetwork.symbol}</TransactionTypeText>
                    <TransactionText>{formatFiatAmountWithCommasAndDecimals(transactionDetails.gasFeeFiat, defaultCurrencies.fiat)}</TransactionText>
                  </SectionRightColumn>
                </SectionRow>
                <Divider />
                <SectionColumn>
                  <TransactionText>{getLocale('braveWalletConfirmTransactionAmountGas')}</TransactionText>
                  <SingleRow>
                    <TransactionTitle>{getLocale('braveWalletConfirmTransactionTotal')}</TransactionTitle>
                    <GrandTotalText>
                      {(transactionInfo.txType !== BraveWallet.TransactionType.ERC721SafeTransferFrom &&
                        transactionInfo.txType !== BraveWallet.TransactionType.ERC721TransferFrom)
                        ? formatWithCommasAndDecimals(transactionDetails.valueExact)
                        : transactionDetails.valueExact
                      } {transactionDetails.symbol} + {formatBalance(transactionDetails.gasFee, selectedNetwork.decimals)} {selectedNetwork.symbol}</GrandTotalText>
                  </SingleRow>
                  <TransactionText
                    hasError={transactionDetails.insufficientFundsError}
                  >
                    {transactionDetails.insufficientFundsError
                      ? `${getLocale('braveWalletSwapInsufficientBalance')} `
                      : ''}
                    {formatFiatAmountWithCommasAndDecimals(transactionDetails.fiatTotal, defaultCurrencies.fiat)}
                  </TransactionText>

                </SectionColumn>
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
      {transactionDetails.contractAddressError &&
        <ErrorText>
          {transactionDetails.contractAddressError}
        </ErrorText>
      }

      {transactionDetails.sameAddressError &&
        <ErrorText>
          {transactionDetails.sameAddressError}
        </ErrorText>
      }

      {transactionDetails.missingGasLimitError &&
        <ErrorText>
          {transactionDetails.missingGasLimitError}
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
          disabled={isConfirmButtonDisabled}
        />
      </ButtonRow>
    </StyledWrapper>
  )
}

export default ConfirmTransactionPanel
