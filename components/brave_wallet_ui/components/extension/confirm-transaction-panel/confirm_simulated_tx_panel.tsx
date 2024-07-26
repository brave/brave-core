// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'
import Alert from '@brave/leo/react/alert'

// Types
import { BraveWallet } from '../../../constants/types'

// Utils
import { getLocale } from '../../../../common/locale'
import { reduceAddress } from '../../../utils/reduce-address'
import {
  isUrlWarning,
  translateSimulationResultError
} from '../../../utils/tx-simulation-utils'
import {
  openAssociatedTokenAccountSupportArticleTab //
} from '../../../utils/routes-utils'

// Hooks
import {
  usePendingTransactions //
} from '../../../common/hooks/use-pending-transaction'
import {
  useGetActiveOriginQuery,
  useGetAddressByteCodeQuery
} from '../../../common/slices/api.slice'
import { useAccountQuery } from '../../../common/slices/api.slice.extra'

// Components
import { AdvancedTransactionSettings } from '../advanced-transaction-settings'
import { EditAllowance } from '../edit-allowance/index'
import { EditPendingTransactionGas } from './common/gas'
import { TransactionOrigin } from './common/origin'
import { PanelTab } from '../panel-tab/index'
import { TransactionDetailBox } from '../transaction-box/index'
import { TransactionQueueSteps } from './common/queue'
import { TransactionSimulationInfo } from './transaction_simulation_info'
import {
  PendingTransactionNetworkFeeAndSettings //
} from '../pending-transaction-network-fee/pending-transaction-network-fee'
import {
  AdvancedTransactionSettingsButton //
} from '../advanced-transaction-settings/button'
import {
  LoadingSimulation //
} from '../enable_transaction_simulations/enable_transaction_simulations'
import CopyTooltip from '../../shared/copy-tooltip/copy-tooltip'
import { CriticalWarningPopup } from './critical-warning-popup'
import {
  PendingTransactionActionsFooter //
} from './common/pending_tx_actions_footer'

// Styled Components
import { Column, ErrorText, Row } from '../../shared/style'
import { NetworkFeeRow } from './common/style'
import {
  AccountNameAndAddress,
  NetworkNameText,
  SimulatedTxMessageBox
} from './confirm_simulated_tx_panel.styles'
import { LearnMoreButton } from '../shared-panel-styles'

type confirmPanelTabs = 'transaction' | 'details'

type Props =
  | {
      simulationType: 'EVM'
      txSimulation: BraveWallet.EVMSimulationResponse
    }
  | {
      simulationType: 'SVM'
      txSimulation: BraveWallet.SolanaSimulationResponse
    }

/**
 * @returns For EVM & Solana transactions
 */
export const ConfirmSimulatedTransactionPanel = ({
  txSimulation,
  simulationType
}: Props) => {
  // custom hooks
  const {
    isSolanaTransaction,
    onEditAllowanceSave,
    transactionDetails,
    transactionsNetwork,
    updateUnapprovedTransactionNonce,
    selectedPendingTransaction,
    onReject,
    queueNextTransaction,
    transactionQueueNumber,
    transactionsQueueLength,
    isConfirmButtonDisabled,
    rejectAllTransactions,
    insufficientFundsForGasError,
    insufficientFundsError,
    onConfirm,
    isSolanaDappTransaction
  } = usePendingTransactions()

  // queries
  const { data: activeOrigin = { eTldPlusOne: '', originSpec: '' } } =
    useGetActiveOriginQuery()

  const { data: byteCode, isLoading } = useGetAddressByteCodeQuery(
    transactionDetails && simulationType === 'EVM'
      ? {
          address: transactionDetails.recipient ?? '',
          coin: transactionDetails.coinType ?? -1,
          chainId: transactionDetails.chainId ?? ''
        }
      : skipToken
  )
  const { account: fromAccount } = useAccountQuery(
    selectedPendingTransaction?.fromAccountId ?? skipToken
  )

  // computed
  const isContract =
    isSolanaDappTransaction || (!isLoading && byteCode && byteCode !== '0x')
  const contractAddress =
    transactionDetails?.recipient && isContract
      ? transactionDetails.recipient
      : undefined
  const originInfo = selectedPendingTransaction?.originInfo ?? activeOrigin

  // state
  const [isCriticalWarningPopupOpen, setIsCriticalWarningPopupOpen] =
    React.useState<boolean>(
      txSimulation.action === BraveWallet.BlowfishSuggestedAction.kBlock
    )
  const [selectedTab, setSelectedTab] =
    React.useState<confirmPanelTabs>('transaction')
  const [isEditing, setIsEditing] = React.useState<boolean>(false)
  const [isEditingAllowance, setIsEditingAllowance] =
    React.useState<boolean>(false)
  const [showAdvancedTransactionSettings, setShowAdvancedTransactionSettings] =
    React.useState<boolean>(false)
  const [isWarningCollapsed, setIsWarningCollapsed] =
    React.useState<boolean>(true)

  // computed
  const simulationResultsErrorText = translateSimulationResultError(
    txSimulation?.error,
    simulationType === 'EVM'
      ? BraveWallet.CoinType.ETH
      : BraveWallet.CoinType.SOL
  )

  const simulationHasOutgoingSPLandSOLTransfers =
    simulationType === 'SVM' &&
    txSimulation.expectedStateChanges.some(
      (sim) =>
        sim.rawInfo.kind ===
          BraveWallet.BlowfishSolanaRawInfoKind.kSplTransfer &&
        sim.rawInfo.data.splTransferData?.diff.sign ===
          BraveWallet.BlowfishDiffSign.kMinus
    ) &&
    txSimulation.expectedStateChanges.some(
      (sim) =>
        sim.rawInfo.kind ===
          BraveWallet.BlowfishSolanaRawInfoKind.kSolTransfer &&
        sim.rawInfo.data.solTransferData?.diff.sign ===
          BraveWallet.BlowfishDiffSign.kMinus
    )

  // methods
  const onSelectTab = React.useCallback(
    (tab: confirmPanelTabs) => () => setSelectedTab(tab),
    []
  )

  const onToggleEditGas = React.useCallback(
    () => setIsEditing((prev) => !prev),
    []
  )

  const onToggleAdvancedTransactionSettings = React.useCallback(() => {
    setShowAdvancedTransactionSettings((prev) => !prev)
  }, [])

  // render
  if (
    !txSimulation ||
    !transactionsNetwork ||
    !transactionDetails ||
    !selectedPendingTransaction
  ) {
    return <LoadingSimulation />
  }

  if (isEditing) {
    return <EditPendingTransactionGas onCancel={onToggleEditGas} />
  }

  if (isEditingAllowance) {
    return (
      <EditAllowance
        onCancel={() => setIsEditingAllowance((prev) => !prev)}
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

  // Critical Warning pop-up
  if (isCriticalWarningPopupOpen) {
    return (
      <CriticalWarningPopup
        onCancel={onReject}
        onProceed={() => setIsCriticalWarningPopupOpen(false)}
      />
    )
  }

  return (
    <Column
      width={'100%'}
      height={'100%'}
      alignItems='center'
      justifyContent='space-between'
    >
      <Column
        gap={'8px'}
        alignItems='center'
        justifyContent='flex-start'
        fullWidth
      >
        <Row
          width='100%'
          padding={'8px 12px'}
          alignItems='center'
          justifyContent={
            transactionsQueueLength > 1 ? 'space-between' : 'flex-start'
          }
        >
          <Column
            alignItems='flex-start'
            gap={'4px'}
          >
            <NetworkNameText>
              {transactionsNetwork?.chainName ?? ''}
            </NetworkNameText>
            <AccountNameAndAddress>
              <CopyTooltip
                isAddress
                text={fromAccount?.accountId.address}
                tooltipText={fromAccount?.accountId.address}
              >
                {fromAccount?.name ?? ''}{' '}
                {reduceAddress(fromAccount?.accountId.address) ?? ''}
              </CopyTooltip>
            </AccountNameAndAddress>
          </Column>
          <TransactionQueueSteps
            queueNextTransaction={queueNextTransaction}
            transactionQueueNumber={transactionQueueNumber}
            transactionsQueueLength={transactionsQueueLength}
          />
        </Row>

        <Row
          alignItems={'center'}
          justifyContent={'center'}
          padding={'0px 24px'}
          width={'100%'}
        >
          <TransactionOrigin
            originInfo={originInfo}
            contractAddress={contractAddress}
            network={transactionsNetwork}
            isFlagged={txSimulation.warnings.some((w) => isUrlWarning(w.kind))}
          />
        </Row>

        <Column
          alignItems={'flex-start'}
          justifyContent={'center'}
          width={'100%'}
          padding={'0px 24px'}
        >
          <Row
            alignItems={'flex-end'}
            justifyContent={'center'}
            width={'100%'}
            marginBottom={'10px'}
          >
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
            {!isSolanaTransaction && (
              <AdvancedTransactionSettingsButton
                onSubmit={onToggleAdvancedTransactionSettings}
              />
            )}
          </Row>

          <Column
            alignItems={'flex-start'}
            justifyContent={'center'}
            width={'100%'}
          >
            {selectedTab === 'transaction' ? (
              simulationResultsErrorText ? (
                <SimulatedTxMessageBox isDetails={true}>
                  <ErrorText>{simulationResultsErrorText}</ErrorText>
                </SimulatedTxMessageBox>
              ) : txSimulation && transactionsNetwork ? (
                simulationType === 'EVM' ? (
                  <TransactionSimulationInfo
                    key={'EVMSimulationInfo'}
                    simulation={txSimulation}
                    simulationType={simulationType}
                    network={transactionsNetwork}
                    transactionDetails={transactionDetails}
                  />
                ) : (
                  <TransactionSimulationInfo
                    key={'SolanaSimulationInfo'}
                    simulation={txSimulation}
                    simulationType={simulationType}
                    network={transactionsNetwork}
                    transactionDetails={transactionDetails}
                  />
                )
              ) : null
            ) : (
              <SimulatedTxMessageBox isDetails={selectedTab === 'details'}>
                <TransactionDetailBox
                  transactionInfo={selectedPendingTransaction}
                  instructions={transactionDetails.instructions}
                />
              </SimulatedTxMessageBox>
            )}
            <NetworkFeeRow>
              <PendingTransactionNetworkFeeAndSettings
                onToggleEditGas={onToggleEditGas}
                feeDisplayMode='fiat'
              />
            </NetworkFeeRow>

            {/*
              Show a warning about a possible token account creation fee
              when SPL tokens are sent
            */}
            {simulationHasOutgoingSPLandSOLTransfers && (
              <Alert type='warning'>
                <>
                  {getLocale(
                    'braveWalletTransactionMayIncludeAccountCreationFee'
                  )}{' '}
                  <LearnMoreButton
                    onClick={openAssociatedTokenAccountSupportArticleTab}
                  >
                    {getLocale('braveWalletAllowAddNetworkLearnMoreButton')}
                  </LearnMoreButton>
                </>
              </Alert>
            )}
          </Column>
        </Column>
      </Column>

      <Column
        fullWidth
        padding={'0px 16px'}
      >
        <PendingTransactionActionsFooter
          isConfirmButtonDisabled={
            isConfirmButtonDisabled || Boolean(simulationResultsErrorText)
          }
          rejectAllTransactions={rejectAllTransactions}
          transactionDetails={transactionDetails}
          transactionsQueueLength={transactionsQueueLength}
          onReject={onReject}
          onConfirm={onConfirm}
          blowfishWarnings={txSimulation.warnings}
          insufficientFundsError={insufficientFundsError}
          insufficientFundsForGasError={insufficientFundsForGasError}
          isWarningCollapsed={isWarningCollapsed}
          setIsWarningCollapsed={setIsWarningCollapsed}
        />
      </Column>
    </Column>
  )
}

export default ConfirmSimulatedTransactionPanel
