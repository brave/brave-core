// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/* eslint-disable @typescript-eslint/key-spacing */

import * as React from 'react'

// types
import { BraveWallet } from '../../../constants/types'

// utils
import { getLocale } from '../../../../common/locale'

// components
import {
  getComponentForEvmApproval,
  getComponentForEvmTransfer
} from './common/evm_state_changes'
import {
  getComponentForSvmTransfer,
  SolStakingAuthChange,
  SPLTokenApproval
} from './common/svm_state_changes'

// style
import {
  groupSimulatedEVMStateChanges,
  decodeSimulatedSVMStateChanges
} from '../../../utils/tx-simulation-utils'
import {
  CollapseHeaderDivider,
  Divider,
  TransactionChangeCollapse,
  TransactionChangeCollapseContainer,
  TransactionChangeCollapseContent,
  TransactionChangeCollapseTitle
} from './confirm_simulated_tx_panel.styles'
import { ChainInfo } from './common/view_on_explorer_button'
import { StateChangeText } from './common/state_changes.styles'

type TransactionInfoProps = (
  | {
      simulationType: 'EVM'
      simulation: BraveWallet.EVMSimulationResponse
    }
  | {
      simulationType: 'SVM'
      simulation: BraveWallet.SolanaSimulationResponse
    }
) & {
  network: ChainInfo
}

export const TransactionSimulationInfo = ({
  simulation,
  simulationType,
  network
}: TransactionInfoProps) => {
  // computed
  const { expectedStateChanges } = simulation

  // memos
  const { evmChanges, svmChanges } = React.useMemo(() => {
    return simulationType === 'EVM'
      ? {
          evmChanges: groupSimulatedEVMStateChanges(
            expectedStateChanges as BraveWallet.BlowfishEVMStateChange[]
          ),
          svmChanges: undefined
        }
      : {
          svmChanges: decodeSimulatedSVMStateChanges(
            expectedStateChanges as BraveWallet.BlowfishSolanaStateChange[]
          ),
          evmChanges: undefined
        }
  }, [simulationType, expectedStateChanges])

  // computed
  const hasApprovals = Boolean(
    evmChanges?.evmApprovals.length || svmChanges?.splApprovals.length
  )

  const hasTransfers = Boolean(
    evmChanges?.evmTransfers.length || svmChanges?.svmTransfers.length
  )

  const hasSolStakingAuthChanges = Boolean(
    svmChanges?.solStakeAuthorityChanges.length
  )

  const hasMultipleCategories =
    [hasApprovals, hasTransfers, hasSolStakingAuthChanges].filter(Boolean)
      .length > 1

  // state
  const [isTransfersSectionOpen, setTransfersSectionOpen] = React.useState(true)
  const [isApprovalsSectionOpen, setIsApprovalsSectionOpen] = React.useState(
    !hasMultipleCategories
  )
  const [isSolStakingAuthSectionOpen, setIsSolStakingAuthSectionOpen] =
    React.useState(!hasMultipleCategories)

  // methods
  const onToggleTransfersSection = React.useCallback(() => {
    if (!hasMultipleCategories) {
      return
    }

    if (isTransfersSectionOpen) {
      setTransfersSectionOpen(false)
      return
    }

    setTransfersSectionOpen(true)
    setIsApprovalsSectionOpen(false)
    setIsSolStakingAuthSectionOpen(false)
  }, [hasMultipleCategories, isTransfersSectionOpen])

  const onToggleApprovalsSection = React.useCallback(() => {
    if (!hasMultipleCategories) {
      return
    }

    if (isApprovalsSectionOpen) {
      setIsApprovalsSectionOpen(false)
      return
    }

    setIsApprovalsSectionOpen(true)

    setTransfersSectionOpen(false)
    setIsSolStakingAuthSectionOpen(false)
  }, [hasMultipleCategories, isApprovalsSectionOpen])

  const onToggleSolStakingAuthSection = React.useCallback(() => {
    if (!hasMultipleCategories) {
      return
    }

    if (isSolStakingAuthSectionOpen) {
      setIsSolStakingAuthSectionOpen(false)
      return
    }

    setIsSolStakingAuthSectionOpen(true)

    setTransfersSectionOpen(false)
    setIsApprovalsSectionOpen(false)
  }, [hasMultipleCategories, isSolStakingAuthSectionOpen])

  // render
  return (
    <TransactionChangeCollapseContainer
      hasMultipleCategories={hasMultipleCategories}
    >
      {/* Transferred Assets */}
      {hasTransfers ? (
        <TransactionChangeCollapse
          onToggle={onToggleTransfersSection}
          hasMultipleCategories={hasMultipleCategories}
          isOpen={isTransfersSectionOpen}
          key={'transfers'}
        >
          <TransactionChangeCollapseTitle slot='title'>
            {getLocale('braveWalletEstimatedBalanceChange')}
          </TransactionChangeCollapseTitle>
          <TransactionChangeCollapseContent>
            <CollapseHeaderDivider key={'Transfers-Divider'} />

            {evmChanges?.evmTransfers.map((transfer, i, arr) => {
              return (
                <React.Fragment key={'EVM-Transfer-' + i}>
                  {getComponentForEvmTransfer(transfer, network)}
                  {i < arr.length - 1 ? <Divider /> : null}
                </React.Fragment>
              )
            })}

            {svmChanges?.svmTransfers.map((transfer, i, arr) => (
              <React.Fragment key={'SVM-Transfer' + i}>
                {getComponentForSvmTransfer(transfer, network)}
                {i < arr.length - 1 ? <Divider /> : null}
              </React.Fragment>
            ))}
          </TransactionChangeCollapseContent>
        </TransactionChangeCollapse>
      ) : null}

      {/* Approvals */}
      {hasApprovals && (
        <TransactionChangeCollapse
          onToggle={onToggleApprovalsSection}
          hasMultipleCategories={hasMultipleCategories}
          isOpen={isApprovalsSectionOpen}
          key='approvals'
        >
          <TransactionChangeCollapseTitle slot='title'>
            {getLocale('braveWalletApprovalDetails')}
          </TransactionChangeCollapseTitle>
          <TransactionChangeCollapseContent>
            <CollapseHeaderDivider key={'EVM-Approvals-Divider'} />

            {evmChanges?.evmApprovals.map((approval, i, arr) => (
              <React.Fragment key={'EVM-Token-Approval-' + i}>
                {getComponentForEvmApproval(approval, network)}
                {i < arr.length - 1 ? <Divider /> : null}
              </React.Fragment>
            ))}

            {svmChanges?.splApprovals.map((approval, i, arr) =>
              approval.rawInfo.data.splApprovalData ? (
                <React.Fragment key={'SVM-Token-Approval-' + i}>
                  <SPLTokenApproval
                    network={network}
                    approval={approval.rawInfo.data.splApprovalData}
                  />
                  {i < arr.length - 1 ? <Divider /> : null}
                </React.Fragment>
              ) : null
            )}
          </TransactionChangeCollapseContent>
        </TransactionChangeCollapse>
      )}

      {/* Staking */}
      {hasSolStakingAuthChanges && (
        <TransactionChangeCollapse
          onToggle={onToggleSolStakingAuthSection}
          hasMultipleCategories={hasMultipleCategories}
          isOpen={isSolStakingAuthSectionOpen}
          key='SOL-staking-changes'
        >
          <TransactionChangeCollapseTitle slot='title'>
            {getLocale('braveWalletAuthorityChange')}
          </TransactionChangeCollapseTitle>
          <TransactionChangeCollapseContent>
            <CollapseHeaderDivider key={'SolStakingAuthChanges-Divider'} />
            {svmChanges?.solStakeAuthorityChanges.map((approval, i, arr) =>
              approval.rawInfo.data.solStakeAuthorityChangeData ? (
                <React.Fragment key={'SolStakingAuthChanges-' + i}>
                  <SolStakingAuthChange
                    authChange={
                      approval.rawInfo.data.solStakeAuthorityChangeData
                    }
                    network={network}
                  />
                  {i < arr.length - 1 ? <Divider /> : null}
                </React.Fragment>
              ) : null
            )}
          </TransactionChangeCollapseContent>
        </TransactionChangeCollapse>
      )}

      {/* No Changes */}
      {!hasTransfers && !hasApprovals && !hasSolStakingAuthChanges && (
        <TransactionChangeCollapse
          onToggle={onToggleSolStakingAuthSection}
          hasMultipleCategories={hasMultipleCategories}
          isOpen={isSolStakingAuthSectionOpen}
          key='SOL-staking-changes'
        >
          <TransactionChangeCollapseTitle slot='title'>
            {getLocale('braveWalletNoChanges')}
          </TransactionChangeCollapseTitle>
          <TransactionChangeCollapseContent>
            <CollapseHeaderDivider key={'NoChanges-Divider'} />
            <StateChangeText>
              {getLocale('braveWalletNoChangesDetected')}
            </StateChangeText>
          </TransactionChangeCollapseContent>
        </TransactionChangeCollapse>
      )}
    </TransactionChangeCollapseContainer>
  )
}
