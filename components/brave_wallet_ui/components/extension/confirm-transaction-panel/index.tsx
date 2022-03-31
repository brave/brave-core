import * as React from 'react'
import { useSelector } from 'react-redux'

import { WalletState } from '../../../constants/types'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import { reduceNetworkDisplayName } from '../../../utils/network-utils'
import Amount from '../../../utils/amount'
import { getLocale } from '../../../../common/locale'

// Hooks
import { usePendingTransactions } from '../../../common/hooks/use-pending-transaction'

// Components
import { CreateSiteOrigin } from '../../shared'

// Components
import { NavButton, PanelTab, TransactionDetailBox } from '../'
import EditGas, { MaxPriorityPanels } from '../edit-gas'
import EditAllowance from '../edit-allowance'
import AdvancedTransactionSettingsButton from '../advanced-transaction-settings/button'
import AdvancedTransactionSettings from '../advanced-transaction-settings'
import { Erc20ApproveTransactionInfo } from './erc-twenty-transaction-info'
import { TransactionInfo } from './transaction-info'

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
  TransactionTypeText,
  ButtonRow,
  AccountCircleWrapper,
  ArrowIcon,
  FromToRow,
  EditButton,
  FavIcon,
  QueueStepText,
  QueueStepRow,
  QueueStepButton,
  ErrorText,
  WarningBox,
  WarningIcon,
  WarningTitle,
  WarningTitleRow
} from './style'
import { Skeleton } from '../../shared/loading-skeleton/styles'

import {
  TabRow,
  Description,
  PanelTitle,
  AccountCircle,
  AddressAndOrb,
  AddressText,
  URLText
} from '../shared-panel-styles'

type confirmPanelTabs = 'transaction' | 'details'

export interface Props {
  onConfirm: () => void
  onReject: () => void
}

function ConfirmTransactionPanel ({
  onConfirm,
  onReject
}: Props) {
  // redux
  const {
    activeOrigin: originInfo,
    defaultCurrencies,
    selectedPendingTransaction: transactionInfo
  } = useSelector(({ wallet }: { wallet: WalletState }) => wallet)

  // custom hooks
  const {
    AssetIconWithPlaceholder,
    baseFeePerGas,
    findAssetPrice,
    foundTokenInfoByContractAddress,
    fromAccountName,
    fromOrb,
    isConfirmButtonDisabled,
    isERC20Approve,
    isERC721SafeTransferFrom,
    isERC721TransferFrom,
    isSolanaTransaction,
    onEditAllowanceSave,
    queueNextTransaction,
    rejectAllTransactions,
    suggestedMaxPriorityFeeChoices,
    toOrb,
    transactionDetails,
    transactionQueueNumber,
    transactionsNetwork,
    transactionsQueueLength,
    transactionTitle,
    updateUnapprovedTransactionGasFields,
    updateUnapprovedTransactionNonce
  } = usePendingTransactions()

  // state
  const [suggestedSliderStep, setSuggestedSliderStep] = React.useState<string>('1')
  const [selectedTab, setSelectedTab] = React.useState<confirmPanelTabs>('transaction')
  const [isEditing, setIsEditing] = React.useState<boolean>(false)
  const [isEditingAllowance, setIsEditingAllowance] = React.useState<boolean>(false)
  const [showAdvancedTransactionSettings, setShowAdvancedTransactionSettings] = React.useState<boolean>(false)
  const [maxPriorityPanel, setMaxPriorityPanel] = React.useState<MaxPriorityPanels>(MaxPriorityPanels.setSuggested)

  // methods
  const onSelectTab = (tab: confirmPanelTabs) => () => setSelectedTab(tab)

  const onToggleEditGas = () => setIsEditing(!isEditing)

  const onToggleEditAllowance = () => setIsEditingAllowance(!isEditingAllowance)

  const onToggleAdvancedTransactionSettings = () => {
    setShowAdvancedTransactionSettings(!showAdvancedTransactionSettings)
  }

  // render
  if (!transactionDetails || !transactionInfo) {
    return <StyledWrapper>
      <Skeleton width={'100%'} height={'100%'} enableAnimation />
    </StyledWrapper>
  }

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
        {isERC20Approve &&
          <AddressAndOrb>
            <AddressText>{reduceAddress(transactionDetails.recipient)}</AddressText>
            <AccountCircle orb={toOrb} />
          </AddressAndOrb>
        }
        {transactionsQueueLength > 1 &&
          <QueueStepRow>
            <QueueStepText>
              {transactionQueueNumber} {getLocale('braveWalletQueueOf')} {transactionsQueueLength}
            </QueueStepText>
            <QueueStepButton
              onClick={queueNextTransaction}
            >
              {transactionQueueNumber === transactionsQueueLength
                ? getLocale('braveWalletQueueFirst')
                : getLocale('braveWalletQueueNext')
              }
            </QueueStepButton>
          </QueueStepRow>
        }
      </TopRow>

      {isERC20Approve ? (
        <>
          <FavIcon src={`chrome://favicon/size/64@1x/${originInfo.originSpec}`} />
          <URLText>
            <CreateSiteOrigin 
              originSpec={originInfo.originSpec}
              eTldPlusOne={originInfo.eTldPlusOne}
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
            <AccountNameText>{fromAccountName}</AccountNameText>
            <ArrowIcon />
            <AccountNameText>{reduceAddress(transactionDetails.recipient)}</AccountNameText>
          </FromToRow>

          <TransactionTypeText>{transactionTitle}</TransactionTypeText>

          {(isERC721TransferFrom || isERC721SafeTransferFrom) &&
            <AssetIconWithPlaceholder
              asset={transactionDetails.erc721BlockchainToken}
              network={transactionsNetwork}
            />
          }

          <TransactionAmountBig>
            {(isERC721TransferFrom || isERC721SafeTransferFrom)
              ? transactionDetails.erc721BlockchainToken?.name + ' ' + transactionDetails.erc721TokenId
              : new Amount(transactionDetails.valueExact)
                .formatAsAsset(undefined, transactionDetails.symbol)
            }
          </TransactionAmountBig>

          {(!isERC721TransferFrom && !isERC721SafeTransferFrom) &&
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
        {!isSolanaTransaction &&
          <AdvancedTransactionSettingsButton
            onSubmit={onToggleAdvancedTransactionSettings}
          />
        }
      </TabRow>

      <MessageBox
        isDetails={selectedTab === 'details'}
        isApprove={isERC20Approve}
      >
        {selectedTab === 'transaction' ? (
          <>
            {isERC20Approve && <Erc20ApproveTransactionInfo onToggleEditGas={onToggleEditGas} />}
            {!isERC20Approve && <TransactionInfo onToggleEditGas={onToggleEditGas} />}
          </>
        ) : <TransactionDetailBox transactionInfo={transactionInfo} />}
      </MessageBox>

      {transactionsQueueLength > 1 &&
        <QueueStepButton
          needsMargin={true}
          onClick={rejectAllTransactions}
        >
          {getLocale('braveWalletQueueRejectAll').replace('$1', transactionsQueueLength.toString())}
        </QueueStepButton>
      }

      {
        [
          transactionDetails.contractAddressError,
          transactionDetails.sameAddressError,
          transactionDetails.missingGasLimitError
        ].map(error => <ErrorText key={error}>{error}</ErrorText>)
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
