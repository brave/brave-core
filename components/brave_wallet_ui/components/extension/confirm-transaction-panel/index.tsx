import * as React from 'react'
import { create } from 'ethereum-blockies'
import { useSelector, useDispatch } from 'react-redux'
import { WalletActions } from '../../../common/actions'

import {
  BraveWallet,
  WalletAccountType,
  DefaultCurrencies,
  OriginInfo,
  WalletState
} from '../../../constants/types'
import {
  UpdateUnapprovedTransactionGasFieldsType,
  UpdateUnapprovedTransactionNonceType,
  UpdateUnapprovedTransactionSpendAllowanceType
} from '../../../common/constants/action_types'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import { reduceNetworkDisplayName, getNetworkFromTXDataUnion } from '../../../utils/network-utils'
import { reduceAccountDisplayName } from '../../../utils/reduce-account-name'
import Amount from '../../../utils/amount'

// Hooks
import { usePricing, useTransactionParser, useTokenInfo } from '../../../common/hooks'
import { useLib } from '../../../common/hooks/useLib'

import { getLocale } from '../../../../common/locale'

// Components
import { withPlaceholderIcon, CreateSiteOrigin } from '../../shared'
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
  TransactionAmountBig,
  TransactionFiatAmountBig,
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
  EditButton,
  FavIcon,
  QueueStepText,
  QueueStepRow,
  QueueStepButton,
  AssetIcon,
  ErrorText,
  WarningBox,
  WarningIcon,
  WarningTitle,
  WarningTitleRow
} from './style'

import {
  TabRow,
  Description,
  PanelTitle,
  AccountCircle,
  AddressAndOrb,
  AddressText,
  URLText
} from '../shared-panel-styles'
import AdvancedTransactionSettingsButton from '../advanced-transaction-settings/button'
import AdvancedTransactionSettings from '../advanced-transaction-settings'

export type confirmPanelTabs = 'transaction' | 'details'

export interface Props {
  originInfo: OriginInfo
  accounts: WalletAccountType[]
  visibleTokens: BraveWallet.BlockchainToken[]
  fullTokenList: BraveWallet.BlockchainToken[]
  transactionInfo: BraveWallet.TransactionInfo
  selectedNetwork: BraveWallet.NetworkInfo
  transactionSpotPrices: BraveWallet.AssetPrice[]
  gasEstimates: BraveWallet.GasEstimation1559 | undefined
  transactionsQueueLength: number
  transactionQueueNumber: number
  defaultCurrencies: DefaultCurrencies
  onQueueNextTransaction: () => void
  onConfirm: () => void
  onReject: () => void
  onRejectAllTransactions: () => void
  getERC20Allowance: (recipient: string, sender: string, approvalTarget: string) => Promise<string>
  updateUnapprovedTransactionGasFields: (payload: UpdateUnapprovedTransactionGasFieldsType) => void
  updateUnapprovedTransactionSpendAllowance: (payload: UpdateUnapprovedTransactionSpendAllowanceType) => void
  updateUnapprovedTransactionNonce: (payload: UpdateUnapprovedTransactionNonceType) => void
}

function ConfirmTransactionPanel (props: Props) {
  const {
    originInfo,
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
    getERC20Allowance,
    updateUnapprovedTransactionGasFields,
    updateUnapprovedTransactionSpendAllowance,
    updateUnapprovedTransactionNonce
  } = props
  const {
    solFeeEstimates,
    defaultNetworks
  } = useSelector(({ wallet }: { wallet: WalletState }) => wallet)
  const dispatch = useDispatch()

  const transactionsNetwork = React.useMemo(() => {
    return getNetworkFromTXDataUnion(transactionInfo.txDataUnion, defaultNetworks, selectedNetwork)
  }, [defaultNetworks, transactionInfo, selectedNetwork])

  const transactionGasEstimates = transactionInfo.txDataUnion.ethTxData1559?.gasEstimation

  const { getBlockchainTokenInfo } = useLib()
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
  const parseTransaction = useTransactionParser(transactionsNetwork, accounts, transactionSpotPrices, visibleTokens, fullTokenList, solFeeEstimates)
  const transactionDetails = parseTransaction(transactionInfo)

  const {
    onFindTokenInfoByContractAddress,
    foundTokenInfoByContractAddress
  } = useTokenInfo(getBlockchainTokenInfo, visibleTokens, fullTokenList, transactionsNetwork)

  React.useEffect(() => {
    const interval = setInterval(() => {
      dispatch(WalletActions.refreshGasEstimates(transactionInfo))
    }, 15000)

    dispatch(WalletActions.refreshGasEstimates(transactionInfo))
    return () => clearInterval(interval)
  }, [transactionInfo])

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
      const allowance = new Amount(result)
        .divideByDecimals(transactionDetails.decimals)
        .format()
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
      allowance: new Amount(allowance)
        .multiplyByDecimals(transactionDetails.decimals)
        .toHex()
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

  /**
   * This will need updating if we ever switch to using per-locale formatting,
   * since `.` isnt always the decimal seperator
  */
  const transactionValueParts = ((transactionInfo.txType !== BraveWallet.TransactionType.ERC721SafeTransferFrom &&
    transactionInfo.txType !== BraveWallet.TransactionType.ERC721TransferFrom)
    ? new Amount(transactionDetails.valueExact)
      .format(undefined, true)
    : transactionDetails.valueExact).split('.')

  /**
   * Inserts a <wbr /> tag between the integer and decimal portions of the value for wrapping
   * This will need updating if we ever switch to using per-locale formatting
   */
  const transactionValueText = <span>
    {transactionValueParts.map((part, i, { length }) => [
      part,
      ...(i < (length - 1) ? ['.'] : []), // dont add a '.' if last part
      <wbr />
    ])}
  </span>

  if (isEditing) {
    return (
      <EditGas
        transactionInfo={transactionInfo}
        onCancel={onToggleEditGas}
        networkSpotPrice={findAssetPrice(transactionsNetwork.symbol)}
        selectedNetwork={transactionsNetwork}
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
        proposedAllowance={transactionDetails.valueExact}
        symbol={transactionDetails.symbol}
        decimals={transactionDetails.decimals}
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
        <NetworkText>{reduceNetworkDisplayName(transactionsNetwork.chainName)}</NetworkText>
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
          <FavIcon src={`chrome://favicon/size/64@1x/${originInfo.origin}`} />
          <URLText>
            <CreateSiteOrigin
              originInfo={originInfo}
            />
          </URLText>
          <PanelTitle>{getLocale('braveWalletAllowSpendTitle').replace('$1', foundTokenInfoByContractAddress?.symbol ?? '')}</PanelTitle>
          <Description>{getLocale('braveWalletAllowSpendDescription').replace('$1', foundTokenInfoByContractAddress?.symbol ?? '')}</Description>

          {transactionDetails.isApprovalUnlimited &&
            <WarningBox>
              <WarningTitleRow>
                <WarningIcon />
                <WarningTitle>
                  {getLocale('braveWalletAllowSpendUnlimitedWarningTitle')}
                </WarningTitle>
              </WarningTitleRow>
            </WarningBox>
          }

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
            <AssetIconWithPlaceholder asset={transactionDetails.erc721BlockchainToken} network={transactionsNetwork} />
          }
          <TransactionAmountBig>
            {transactionInfo.txType === BraveWallet.TransactionType.ERC721TransferFrom ||
              transactionInfo.txType === BraveWallet.TransactionType.ERC721SafeTransferFrom
              ? transactionDetails.erc721BlockchainToken?.name + ' ' + transactionDetails.erc721TokenId
              : new Amount(transactionDetails.valueExact)
                .formatAsAsset(undefined, transactionDetails.symbol)
            }
          </TransactionAmountBig>
          {transactionInfo.txType !== BraveWallet.TransactionType.ERC721TransferFrom &&
            transactionInfo.txType !== BraveWallet.TransactionType.ERC721SafeTransferFrom &&
            <TransactionFiatAmountBig>
              {
                transactionDetails.fiatValue.formatAsFiat(defaultCurrencies.fiat)
              }
            </TransactionFiatAmountBig>
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
        {transactionInfo.txType !== BraveWallet.TransactionType.SolanaSystemTransfer &&
          <AdvancedTransactionSettingsButton
            onSubmit={onToggleAdvancedTransactionSettings}
          />
        }
      </TabRow>
      <MessageBox isDetails={selectedTab === 'details'} isApprove={transactionInfo.txType === BraveWallet.TransactionType.ERC20Approve}>
        {selectedTab === 'transaction' ? (
          <>
            {transactionInfo.txType === BraveWallet.TransactionType.ERC20Approve &&
              <>
                <SectionRow>
                  <TransactionTitle>{getLocale('braveWalletAllowSpendTransactionFee')}</TransactionTitle>
                  <EditButton onClick={onToggleEditGas}>{getLocale('braveWalletAllowSpendEditButton')}</EditButton>
                </SectionRow>
                <TransactionTypeText>
                  {
                    new Amount(transactionDetails.gasFee)
                      .divideByDecimals(transactionsNetwork.decimals)
                      .formatAsAsset(6, transactionsNetwork.symbol)
                  }
                </TransactionTypeText>
                <TransactionText
                  hasError={transactionDetails.insufficientFundsError}
                >
                  {transactionDetails.insufficientFundsError ? `${getLocale('braveWalletSwapInsufficientBalance')} ` : ''}
                  {new Amount(transactionDetails.gasFeeFiat)
                    .formatAsFiat(defaultCurrencies.fiat)}
                </TransactionText>

                <Divider />

                <TransactionTitle>{getLocale('braveWalletAllowSpendCurrentAllowance')}</TransactionTitle>
                <TransactionTypeText>{currentTokenAllowance} {transactionDetails.symbol}</TransactionTypeText>

                <Divider />

                <TransactionTitle>{getLocale('braveWalletAllowSpendProposedAllowance')}</TransactionTitle>
                <TransactionTypeText>
                  {
                    transactionDetails.isApprovalUnlimited
                      ? getLocale('braveWalletTransactionApproveUnlimited')
                      : new Amount(transactionDetails.valueExact)
                        .formatAsAsset(undefined, transactionDetails.symbol)
                  }
                </TransactionTypeText>

              </>
            }

            {transactionInfo.txType !== BraveWallet.TransactionType.ERC20Approve &&
              <>

                <SectionRow>
                  <TransactionTitle>
                    {transactionInfo.txType === BraveWallet.TransactionType.SolanaSystemTransfer
                      ? getLocale('braveWalletConfirmTransactionTransactionFee')
                      : getLocale('braveWalletConfirmTransactionGasFee')}
                  </TransactionTitle>
                  {transactionInfo.txType !== BraveWallet.TransactionType.SolanaSystemTransfer &&
                    <EditButton onClick={onToggleEditGas}>{getLocale('braveWalletAllowSpendEditButton')}</EditButton>
                  }
                </SectionRow>
                <TransactionTypeText>
                  {
                    new Amount(transactionDetails.gasFee)
                      .divideByDecimals(transactionsNetwork.decimals)
                      .formatAsAsset(6, transactionsNetwork.symbol)
                  }
                </TransactionTypeText>
                <TransactionText>
                  {
                    new Amount(transactionDetails.gasFeeFiat)
                      .formatAsFiat(defaultCurrencies.fiat)
                  }
                </TransactionText>
                <Divider />
                <WarningTitleRow>
                  <TransactionTitle>
                    {getLocale('braveWalletConfirmTransactionTotal')}
                    {' '}
                    ({transactionInfo.txType === BraveWallet.TransactionType.SolanaSystemTransfer
                      ? getLocale('braveWalletConfirmTransactionAmountFee')
                      : getLocale('braveWalletConfirmTransactionAmountGas')})
                  </TransactionTitle>
                </WarningTitleRow>
                <TransactionTypeText>
                  {transactionValueText} {transactionDetails.symbol} +
                </TransactionTypeText>
                <TransactionTypeText>
                  {
                    new Amount(transactionDetails.gasFee)
                      .divideByDecimals(transactionsNetwork.decimals)
                      .formatAsAsset(6, transactionsNetwork.symbol)
                  }
                </TransactionTypeText>

                <TransactionText
                  hasError={transactionDetails.insufficientFundsError}
                >
                  {transactionDetails.insufficientFundsError
                    ? `${getLocale('braveWalletSwapInsufficientBalance')} `
                    : ''}
                  {transactionDetails.fiatTotal
                    .formatAsFiat(defaultCurrencies.fiat)}
                </TransactionText>
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
