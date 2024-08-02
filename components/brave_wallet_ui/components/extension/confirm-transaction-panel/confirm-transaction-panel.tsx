// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import Amount from '../../../utils/amount'
import { getLocale } from '../../../../common/locale'
import {
  openAssociatedTokenAccountSupportArticleTab //
} from '../../../utils/routes-utils'

// Hooks
import { usePendingTransactions } from '../../../common/hooks/use-pending-transaction'
import { useExplorer } from '../../../common/hooks/explorer'
import {
  useGetActiveOriginQuery,
  useGetAddressByteCodeQuery,
  useGetDefaultFiatCurrencyQuery
} from '../../../common/slices/api.slice'

// Components
import CreateSiteOrigin from '../../shared/create-site-origin/index'
import { Tooltip } from '../../shared/tooltip/index'
import { withPlaceholderIcon } from '../../shared/create-placeholder-icon'
import { PanelTab } from '../panel-tab/index'
import { TransactionDetailBox } from '../transaction-box/index'
import { EditAllowance } from '../edit-allowance/index'
import {
  AdvancedTransactionSettingsButton //
} from '../advanced-transaction-settings/button/index'
import {
  AdvancedTransactionSettings //
} from '../advanced-transaction-settings/index'
import { TransactionInfo } from './transaction-info'
import { NftIcon } from '../../shared/nft-icon/nft-icon'
import {
  PendingTransactionActionsFooter //
} from './common/pending_tx_actions_footer'
import { TransactionQueueSteps } from './common/queue'
import { Origin } from './common/origin'
import { EditPendingTransactionGas } from './common/gas'
import { TxWarningBanner } from './common/tx_warnings'
import { LoadingPanel } from '../loading_panel/loading_panel'
import {
  PendingTransactionNetworkFeeAndSettings //
} from '../pending-transaction-network-fee/pending-transaction-network-fee'
import {
  TransactionSimulationNotSupportedSheet //
} from '../transaction_simulation_not_supported_sheet/transaction_simulation_not_supported_sheet'

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
  AccountCircleWrapper,
  ArrowIcon,
  EditButton,
  WarningIcon,
  ContractButton,
  ExplorerIcon,
  WarningInfoCircleIcon
} from './style'

import {
  TabRow,
  Description,
  PanelTitle,
  AddressAndOrb,
  AddressText,
  WarningBox,
  WarningTitle,
  LearnMoreButton,
  WarningBoxTitleRow,
  URLText
} from '../shared-panel-styles'
import { Column, Row } from '../../shared/style'
import { NetworkFeeRow } from './common/style'
import { FooterContainer } from './common/pending_tx_actions_footer.style'

type confirmPanelTabs = 'transaction' | 'details'

const ICON_CONFIG = { size: 'big', marginLeft: 0, marginRight: 0 } as const
const NftAssetIconWithPlaceholder = withPlaceholderIcon(NftIcon, ICON_CONFIG)

export const ConfirmTransactionPanel = ({
  retrySimulation,
  showSimulationNotSupportedMessage
}: {
  readonly retrySimulation?: () => void
  showSimulationNotSupportedMessage?: boolean
}) => {
  // queries
  const { data: activeOrigin = { eTldPlusOne: '', originSpec: '' } } =
    useGetActiveOriginQuery()
  const { data: defaultFiatCurrency = 'usd' } = useGetDefaultFiatCurrencyQuery()

  // custom hooks
  const {
    erc20ApproveTokenInfo,
    fromAccount,
    fromOrb,
    isERC20Approve,
    isERC721SafeTransferFrom,
    isERC721TransferFrom,
    isEthereumTransaction,
    isAssociatedTokenAccountCreation,
    onEditAllowanceSave,
    toOrb,
    transactionDetails,
    transactionsNetwork,
    transactionTitle,
    updateUnapprovedTransactionNonce,
    isCurrentAllowanceUnlimited,
    currentTokenAllowance,
    selectedPendingTransaction,
    onConfirm,
    onReject,
    gasFee,
    insufficientFundsError,
    insufficientFundsForGasError,
    queueNextTransaction,
    transactionQueueNumber,
    transactionsQueueLength,
    isSolanaTransaction,
    isBitcoinTransaction,
    isZCashTransaction,
    hasFeeEstimatesError,
    isLoadingGasFee,
    rejectAllTransactions,
    isConfirmButtonDisabled,
    isSolanaDappTransaction
  } = usePendingTransactions()

  // queries
  const { data: byteCode, isLoading } = useGetAddressByteCodeQuery(
    transactionDetails && isEthereumTransaction
      ? {
          address: transactionDetails.recipient ?? '',
          coin: transactionDetails.coinType ?? -1,
          chainId: transactionDetails.chainId ?? ''
        }
      : skipToken
  )

  // computed
  const isContract =
    !isLoading && isEthereumTransaction && byteCode && byteCode !== '0x'
  const originInfo = selectedPendingTransaction?.originInfo ?? activeOrigin

  // hooks
  const onClickViewOnBlockExplorer = useExplorer(transactionsNetwork)

  // state
  const [selectedTab, setSelectedTab] =
    React.useState<confirmPanelTabs>('transaction')
  const [isSimulationWarningDismissed, setIsSimulationWarningDismissed] =
    React.useState(false)
  const [isEditing, setIsEditing] = React.useState<boolean>(false)
  const [isEditingAllowance, setIsEditingAllowance] =
    React.useState<boolean>(false)
  const [showAdvancedTransactionSettings, setShowAdvancedTransactionSettings] =
    React.useState<boolean>(false)
  const [isWarningCollapsed, setIsWarningCollapsed] = React.useState(true)

  // methods
  const onSelectTab = (tab: confirmPanelTabs) => () => setSelectedTab(tab)

  const onToggleEditGas = () => setIsEditing((prev) => !prev)

  const onToggleEditAllowance = () => setIsEditingAllowance((prev) => !prev)

  const onToggleAdvancedTransactionSettings = () => {
    setShowAdvancedTransactionSettings((prev) => !prev)
  }

  // render
  if (
    !transactionDetails ||
    !selectedPendingTransaction ||
    !fromAccount ||
    !transactionsQueueLength
  ) {
    return <LoadingPanel />
  }

  if (isEditing) {
    return <EditPendingTransactionGas onCancel={onToggleEditGas} />
  }

  if (isEditingAllowance) {
    return (
      <EditAllowance
        onCancel={onToggleEditAllowance}
        onSave={onEditAllowanceSave}
        proposedAllowance={transactionDetails.valueExact}
        symbol={transactionDetails.symbol}
        approvalTarget={transactionDetails.approvalTargetLabel || ''}
        isApprovalUnlimited={transactionDetails.isApprovalUnlimited || false}
      />
    )
  }

  if (showAdvancedTransactionSettings) {
    return (
      <AdvancedTransactionSettings
        onCancel={onToggleAdvancedTransactionSettings}
        nonce={transactionDetails.nonce}
        chainId={selectedPendingTransaction.chainId}
        txMetaId={selectedPendingTransaction.id}
        updateUnapprovedTransactionNonce={updateUnapprovedTransactionNonce}
      />
    )
  }

  return (
    <StyledWrapper>
      <TopRow>
        <NetworkText>{transactionsNetwork?.chainName ?? ''}</NetworkText>

        <TransactionQueueSteps
          queueNextTransaction={queueNextTransaction}
          transactionQueueNumber={transactionQueueNumber}
          transactionsQueueLength={transactionsQueueLength}
        />
      </TopRow>

      {isERC20Approve ? (
        <>
          <Origin originInfo={originInfo} />
          <PanelTitle>
            {getLocale('braveWalletAllowSpendTitle').replace(
              '$1',
              erc20ApproveTokenInfo?.symbol ?? ''
            )}
          </PanelTitle>
          <AddressAndOrb>
            <Tooltip
              text={transactionDetails.approvalTarget}
              isAddress={true}
              position={'right'}
            >
              <AddressText>
                {transactionDetails.approvalTargetLabel}
              </AddressText>
            </Tooltip>
          </AddressAndOrb>
          <Description>
            {getLocale('braveWalletAllowSpendDescription').replace(
              '$1',
              erc20ApproveTokenInfo?.symbol ?? ''
            )}
          </Description>

          {transactionDetails.isApprovalUnlimited && (
            <WarningBox warningType={'danger'}>
              <WarningBoxTitleRow>
                <WarningIcon />
                <WarningTitle warningType={'danger'}>
                  {getLocale('braveWalletAllowSpendUnlimitedWarningTitle')}
                </WarningTitle>
              </WarningBoxTitleRow>
            </WarningBox>
          )}

          <EditButton onClick={onToggleEditAllowance}>
            {getLocale('braveWalletEditPermissionsButton')}
          </EditButton>
        </>
      ) : (
        <>
          <AccountCircleWrapper>
            <FromCircle orb={fromOrb} />
            <ToCircle orb={toOrb} />
          </AccountCircleWrapper>
          <URLText>
            <CreateSiteOrigin
              originSpec={originInfo.originSpec}
              eTldPlusOne={originInfo.eTldPlusOne}
            />
          </URLText>

          <Row
            marginBottom={8}
            maxWidth={isContract ? '90%' : 'unset'}
            width={'100%'}
            gap={'8px'}
            $wrap
          >
            <Tooltip
              text={fromAccount.address}
              isVisible={!!fromAccount.address}
              isAddress={true}
              position={'left'}
            >
              <AccountNameText>{fromAccount.name}</AccountNameText>
            </Tooltip>

            {transactionDetails.recipient &&
              transactionDetails.recipient !== fromAccount.address && (
                <>
                  <ArrowIcon />
                  {isContract ? (
                    <Column
                      alignItems={'flex-start'}
                      justifyContent={'flex-start'}
                    >
                      <NetworkText>
                        {getLocale('braveWalletNFTDetailContractAddress')}
                      </NetworkText>
                      <ContractButton
                        onClick={onClickViewOnBlockExplorer(
                          'contract',
                          `${transactionDetails.recipient}`
                        )}
                      >
                        {reduceAddress(transactionDetails.recipient)}{' '}
                        <ExplorerIcon />
                      </ContractButton>
                    </Column>
                  ) : (
                    <Tooltip
                      text={transactionDetails.recipient}
                      isAddress={true}
                      position='right'
                    >
                      <AccountNameText>
                        {reduceAddress(transactionDetails.recipient)}
                      </AccountNameText>
                    </Tooltip>
                  )}
                </>
              )}
          </Row>

          <TransactionTypeText>{transactionTitle}</TransactionTypeText>

          {(isERC721TransferFrom || isERC721SafeTransferFrom) && (
            <NftAssetIconWithPlaceholder
              asset={transactionDetails.erc721BlockchainToken}
            />
          )}

          {!isSolanaDappTransaction && (
            <>
              <Row
                margin={
                  isAssociatedTokenAccountCreation
                    ? '0px 0px 0px 16px'
                    : undefined
                }
                alignItems='center'
                justifyContent='center'
                gap={'4px'}
              >
                <TransactionAmountBig>
                  {isERC721TransferFrom || isERC721SafeTransferFrom
                    ? transactionDetails.erc721BlockchainToken?.name +
                      ' ' +
                      transactionDetails.erc721TokenId
                    : new Amount(transactionDetails.valueExact).formatAsAsset(
                        undefined,
                        transactionDetails.symbol
                      )}
                </TransactionAmountBig>

                {isAssociatedTokenAccountCreation && (
                  <Tooltip
                    maxWidth={'200px'}
                    minWidth={'180px'}
                    text={
                      <>
                        {getLocale(
                          'braveWalletConfirmTransactionAccountCreationFee'
                        )}{' '}
                        <LearnMoreButton
                          onClick={openAssociatedTokenAccountSupportArticleTab}
                        >
                          {getLocale(
                            'braveWalletAllowAddNetworkLearnMoreButton'
                          )}
                        </LearnMoreButton>
                      </>
                    }
                  >
                    <WarningInfoCircleIcon />
                  </Tooltip>
                )}
              </Row>

              {!isERC721TransferFrom && !isERC721SafeTransferFrom && (
                <TransactionFiatAmountBig>
                  {new Amount(transactionDetails.fiatValue).formatAsFiat(
                    defaultFiatCurrency
                  )}
                </TransactionFiatAmountBig>
              )}
            </>
          )}
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
        {isEthereumTransaction && (
          <AdvancedTransactionSettingsButton
            onSubmit={onToggleAdvancedTransactionSettings}
          />
        )}
      </TabRow>

      <MessageBox isDetails={selectedTab === 'details'}>
        {selectedTab === 'transaction' ? (
          <TransactionInfo
            onToggleEditGas={
              isSolanaTransaction || isBitcoinTransaction
                ? undefined
                : onToggleEditGas
            }
            isZCashTransaction={isZCashTransaction}
            isBitcoinTransaction={isBitcoinTransaction}
            transactionDetails={transactionDetails}
            isERC721SafeTransferFrom={isERC721SafeTransferFrom}
            isERC721TransferFrom={isERC721TransferFrom}
            transactionsNetwork={transactionsNetwork}
            hasFeeEstimatesError={Boolean(hasFeeEstimatesError)}
            isLoadingGasFee={isLoadingGasFee}
            gasFee={gasFee}
            insufficientFundsError={insufficientFundsError}
            insufficientFundsForGasError={insufficientFundsForGasError}
            isERC20Approve={isERC20Approve}
            currentTokenAllowance={currentTokenAllowance}
            isCurrentAllowanceUnlimited={isCurrentAllowanceUnlimited}
          />
        ) : (
          <TransactionDetailBox
            transactionInfo={selectedPendingTransaction}
            instructions={transactionDetails.instructions}
          />
        )}
      </MessageBox>

      <NetworkFeeRow>
        <PendingTransactionNetworkFeeAndSettings
          onToggleEditGas={onToggleEditGas}
          feeDisplayMode='fiat'
        />
      </NetworkFeeRow>

      <Column fullWidth>
        <FooterContainer>
          {retrySimulation &&
            !isSimulationWarningDismissed &&
            !showSimulationNotSupportedMessage && (
              <TxWarningBanner
                retrySimulation={retrySimulation}
                onDismiss={() => setIsSimulationWarningDismissed(true)}
              />
            )}
        </FooterContainer>
        <PendingTransactionActionsFooter
          onConfirm={onConfirm}
          onReject={onReject}
          rejectAllTransactions={rejectAllTransactions}
          isConfirmButtonDisabled={isConfirmButtonDisabled}
          transactionDetails={transactionDetails}
          transactionsQueueLength={transactionsQueueLength}
          insufficientFundsForGasError={insufficientFundsForGasError}
          insufficientFundsError={insufficientFundsError}
          isWarningCollapsed={isWarningCollapsed}
          setIsWarningCollapsed={setIsWarningCollapsed}
        />
      </Column>
      {showSimulationNotSupportedMessage && (
        <TransactionSimulationNotSupportedSheet />
      )}
    </StyledWrapper>
  )
}

export default ConfirmTransactionPanel
