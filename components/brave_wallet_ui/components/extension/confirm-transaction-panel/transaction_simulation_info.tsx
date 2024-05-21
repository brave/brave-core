// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/* eslint-disable @typescript-eslint/key-spacing */

import * as React from 'react'

// types
import { BraveWallet } from '../../../constants/types'
import type { ParsedTransaction } from '../../../utils/tx-utils'

// utils
import { getLocale } from '../../../../common/locale'

// components
import {
  getComponentForEvmApproval,
  getComponentForEvmTransfer
} from './common/evm_state_changes'
import {
  getComponentForSvmTransfer,
  SolAccountOwnershipChange,
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

type CategoryName =
  | 'accountOwnership'
  | 'transfers'
  | 'approvals'
  | 'solStakingAuthChanges'
  | 'noChanges'

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
  transactionDetails?: ParsedTransaction
}

export const TransactionSimulationInfo = ({
  simulation,
  simulationType,
  network,
  transactionDetails
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
  const hasAccountOwnershipChanges = Boolean(
    svmChanges?.accountOwnerChangeData.length
  )

  const hasApprovals = Boolean(
    evmChanges?.evmApprovals.length || svmChanges?.splApprovals.length
  )

  const hasTransfers = Boolean(
    evmChanges?.evmTransfers.length || svmChanges?.svmTransfers.length
  )

  const hasSolStakingAuthChanges = Boolean(
    svmChanges?.solStakeAuthorityChanges.length
  )

  const changesCategories = [
    hasApprovals,
    hasTransfers,
    hasSolStakingAuthChanges,
    hasAccountOwnershipChanges
  ].filter(Boolean)

  const hasMultipleCategories = changesCategories.length > 1
  const hasNoChanges = changesCategories.length === 0

  // state
  const [openedCategorySectionName, setOpenedCategorySectionName] =
    React.useState<CategoryName>(
      hasAccountOwnershipChanges
        ? 'accountOwnership'
        : hasTransfers
        ? 'transfers'
        : hasApprovals
        ? 'approvals'
        : hasSolStakingAuthChanges
        ? 'solStakingAuthChanges'
        : 'noChanges'
    )

  // render
  return (
    <TransactionChangeCollapseContainer
      hasMultipleCategories={hasMultipleCategories}
    >
      {/* Account ownership changes */}
      {hasAccountOwnershipChanges && (
        <TransactionChangeCollapse
          onToggle={
            hasMultipleCategories
              ? () =>
                  setOpenedCategorySectionName((prev) =>
                    prev === 'accountOwnership'
                      ? 'noChanges'
                      : 'accountOwnership'
                  )
              : undefined
          }
          hasMultipleCategories={hasMultipleCategories}
          isOpen={openedCategorySectionName === 'accountOwnership'}
          key='SOL-account-ownership-changes'
        >
          <TransactionChangeCollapseTitle slot='title'>
            {getLocale('braveWalletOwnershipChange')}
          </TransactionChangeCollapseTitle>
          <TransactionChangeCollapseContent>
            <CollapseHeaderDivider key={'SolAccountOwnershipChanges-Divider'} />
            {svmChanges?.accountOwnerChangeData.map((approval, i, arr) =>
              approval.rawInfo.data.userAccountOwnerChangeData ? (
                <React.Fragment key={'SolAccountOwnershipChanges-' + i}>
                  <SolAccountOwnershipChange
                    ownerChange={
                      approval.rawInfo.data.userAccountOwnerChangeData
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

      {/* Transferred Assets */}
      {hasTransfers ? (
        <TransactionChangeCollapse
          onToggle={
            hasMultipleCategories
              ? () =>
                  setOpenedCategorySectionName((prev) =>
                    prev === 'transfers' ? 'noChanges' : 'transfers'
                  )
              : undefined
          }
          hasMultipleCategories={hasMultipleCategories}
          isOpen={openedCategorySectionName === 'transfers'}
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
                {getComponentForSvmTransfer(
                  transfer,
                  network,
                  transactionDetails
                )}
                {i < arr.length - 1 ? <Divider /> : null}
              </React.Fragment>
            ))}
          </TransactionChangeCollapseContent>
        </TransactionChangeCollapse>
      ) : null}

      {/* Approvals */}
      {hasApprovals && (
        <TransactionChangeCollapse
          onToggle={
            hasMultipleCategories
              ? () =>
                  setOpenedCategorySectionName((prev) =>
                    prev === 'approvals' ? 'noChanges' : 'approvals'
                  )
              : undefined
          }
          hasMultipleCategories={hasMultipleCategories}
          isOpen={openedCategorySectionName === 'approvals'}
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
          onToggle={
            hasMultipleCategories
              ? () =>
                  setOpenedCategorySectionName((prev) =>
                    prev === 'solStakingAuthChanges'
                      ? 'noChanges'
                      : 'solStakingAuthChanges'
                  )
              : undefined
          }
          hasMultipleCategories={hasMultipleCategories}
          isOpen={openedCategorySectionName === 'solStakingAuthChanges'}
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
      {hasNoChanges && (
        <TransactionChangeCollapse
          hasMultipleCategories={false}
          isOpen={true}
          key='No-changes'
        >
          <TransactionChangeCollapseTitle slot='title'>
            {getLocale('braveWalletEstimatedBalanceChange')}
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
