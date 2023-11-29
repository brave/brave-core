// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Types
import { BraveWallet } from '../../../constants/types'

// Utils
import { reduceAddress } from '../../../utils/reduce-address'
import { WalletSelectors } from '../../../common/selectors'
import {
  isUrlWarning,
  translateSimulationResultError
} from '../../../utils/tx-simulation-utils'

// Hooks
import {
  usePendingTransactions //
} from '../../../common/hooks/use-pending-transaction'
import { useExplorer } from '../../../common/hooks/explorer'
import {
  useGetAddressByteCodeQuery //
} from '../../../common/slices/api.slice'
import { useAccountQuery } from '../../../common/slices/api.slice.extra'
import {
  useUnsafeWalletSelector //
} from '../../../common/hooks/use-safe-selector'

// Components
import { AdvancedTransactionSettings } from '../advanced-transaction-settings'
import { EditAllowance } from '../edit-allowance/index'
import { EditPendingTransactionGas } from './common/gas'
import { Footer } from './common/footer'
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
import { SimulationWarnings } from './common/tx_simulation_warnings'
import { CriticalWarningPopup } from './critical-warning-popup'

// Styled Components
import { Column, ErrorText, Row } from '../../shared/style'
import { NetworkFeeRow } from './common/style'
import {
  AccountNameAndAddress,
  NetworkNameText,
  OriginRow,
  SimulationInfoColumn,
  TabsAndContent,
  TabRow,
  StyledWrapper,
  SimulatedTxMessageBox
} from './confirm_simulated_tx_panel.styles'

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
  // redux
  const activeOrigin = useUnsafeWalletSelector(WalletSelectors.activeOrigin)

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
    transactionsQueueLength
  } = usePendingTransactions()

  const onClickViewOnBlockExplorer = useExplorer(transactionsNetwork)

  // queries
  const { data: byteCode, isLoading } = useGetAddressByteCodeQuery(
    transactionDetails
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
  const isContract = !isLoading && byteCode !== '0x'
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

  const hasCriticalWarnings = txSimulation.warnings.some(
    (warning) =>
      warning.severity === BraveWallet.BlowfishWarningSeverity.kCritical
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
    <StyledWrapper>
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

        <OriginRow>
          <TransactionOrigin
            originInfo={originInfo}
            contractAddress={contractAddress}
            onClickContractAddress={onClickViewOnBlockExplorer(
              'contract',
              contractAddress
            )}
            isFlagged={txSimulation.warnings.some((w) => isUrlWarning(w.kind))}
          />
        </OriginRow>

        <TabsAndContent>
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
            {!isSolanaTransaction && (
              <AdvancedTransactionSettingsButton
                onSubmit={onToggleAdvancedTransactionSettings}
              />
            )}
          </TabRow>

          <SimulationInfoColumn>
            {isWarningCollapsed && (
              <>
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
                      />
                    ) : (
                      <TransactionSimulationInfo
                        key={'SolanaSimulationInfo'}
                        simulation={txSimulation}
                        simulationType={simulationType}
                        network={transactionsNetwork}
                      />
                    )
                  ) : null
                ) : (
                  <SimulatedTxMessageBox isDetails={selectedTab === 'details'}>
                    <TransactionDetailBox
                      transactionInfo={selectedPendingTransaction}
                    />
                  </SimulatedTxMessageBox>
                )}
                <NetworkFeeRow>
                  <PendingTransactionNetworkFeeAndSettings
                    onToggleEditGas={onToggleEditGas}
                    feeDisplayMode='fiat'
                  />
                </NetworkFeeRow>
              </>
            )}
          </SimulationInfoColumn>
        </TabsAndContent>
      </Column>

      <Column fullWidth>
        <SimulationWarnings
          txSimulation={txSimulation}
          isWarningCollapsed={isWarningCollapsed}
          hasCriticalWarnings={hasCriticalWarnings}
          setIsWarningCollapsed={setIsWarningCollapsed}
        />

        <Footer
          showGasErrors
          disableConfirmation={!!simulationResultsErrorText}
        />
      </Column>
    </StyledWrapper>
  )
}

export default ConfirmSimulatedTransactionPanel
