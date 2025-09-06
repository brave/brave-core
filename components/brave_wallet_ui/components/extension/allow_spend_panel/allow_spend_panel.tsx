// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Tooltip from '@brave/leo/react/tooltip'
import Icon from '@brave/leo/react/icon'

// Hooks
import { useExplorer } from '../../../common/hooks/explorer'

// Utils
import { getLocale, formatLocale } from '../../../../common/locale'
import Amount from '../../../utils/amount'

// Types
import { BraveWallet } from '../../../constants/types'
import { ParsedTransaction } from '../../../utils/tx-utils'

// Components
import { OriginInfoCard } from '../origin_info_card/origin_info_card'
import { EditSpendLimit } from '../edit_spend_permissions/edit_spend_limit'
import { BottomSheet } from '../../shared/bottom_sheet/bottom_sheet'
import {
  ConfirmationFooterActions, //
} from '../confirmation_footer_actions/confirmation_footer_actions'
import {
  ConfirmationNetworkFee, //
} from '../confirmation_network_fee/confirmation_network_fee'
import { ConfirmationHeader } from '../confirmation_header/confirmation_header'

// Styled Components
import {
  StyledWrapper,
  Card,
  Title,
  Description,
  InfoBox,
  AmountText,
  TokenButtonLink,
} from './allow_spend_panel.style'
import { Column, Row, VerticalDivider } from '../../shared/style'
import {
  ConfirmationInfoLabel,
  ConfirmationButtonLink,
} from '../shared-panel-styles'
export interface Props {
  token?: BraveWallet.BlockchainToken
  transactionDetails: ParsedTransaction
  gasFee: string
  network?: BraveWallet.NetworkInfo
  originInfo?: BraveWallet.OriginInfo
  currentLimit?: string
  isCurrentAllowanceUnlimited: boolean
  transactionsQueueLength: number
  onSaveSpendLimit: (limit: string) => void
  onClickDetails: () => void
  onClickAdvancedSettings: () => void
  onClickEditNetworkFee: () => void
  onConfirm: () => void
  onReject: () => void
  queueNextTransaction: () => void
  queuePreviousTransaction: () => void
  rejectAllTransactions: () => void
}

export const AllowSpendPanel = (props: Props) => {
  const {
    token,
    originInfo,
    transactionDetails,
    network,
    currentLimit,
    isCurrentAllowanceUnlimited,
    gasFee,
    transactionsQueueLength,
    onConfirm,
    onReject,
    onClickDetails,
    onClickAdvancedSettings,
    onClickEditNetworkFee,
    onSaveSpendLimit,
    queueNextTransaction,
    queuePreviousTransaction,
    rejectAllTransactions,
  } = props

  // State
  const [showEditSpendLimit, setShowEditSpendLimit] = React.useState(false)

  // Hooks
  const onClickViewOnBlockExplorer = useExplorer(network)

  // Computed
  const isApprovalUnlimited = transactionDetails.isApprovalUnlimited

  return (
    <>
      <StyledWrapper
        width='100%'
        height='100%'
        justifyContent='space-between'
      >
        {/* Header */}
        <ConfirmationHeader
          title={getLocale('braveWalletSpendLimit')}
          transactionsQueueLength={transactionsQueueLength}
          queueNextTransaction={queueNextTransaction}
          queuePreviousTransaction={queuePreviousTransaction}
          rejectAllTransactions={rejectAllTransactions}
        />
        <VerticalDivider />

        {/* Content */}
        <Column
          padding='8px'
          width='100%'
          height='100%'
          justifyContent='flex-start'
          gap='8px'
        >
          <Card
            width='100%'
            height='100%'
            justifyContent='space-between'
          >
            <Column>
              {originInfo && <OriginInfoCard origin={originInfo} />}
              <Column
                padding='16px 16px 0px 16px'
                gap='8px'
              >
                <Title>
                  {formatLocale('braveWalletAllowSpendTitle', {
                    $1: (
                      <Row
                        width='unset'
                        margin='0px 0px 0px 4px'
                      >
                        <Tooltip text={token?.contractAddress ?? ''}>
                          <TokenButtonLink
                            onClick={onClickViewOnBlockExplorer(
                              'token',
                              token?.contractAddress ?? '',
                            )}
                          >
                            {token?.symbol ?? ''}
                          </TokenButtonLink>
                        </Tooltip>
                      </Row>
                    ),
                  })}
                </Title>
                <Description textColor='tertiary'>
                  {getLocale('braveWalletAllowSpendDescription').replace(
                    '$1',
                    token?.symbol ?? '',
                  )}
                </Description>
                <Row>
                  <Button
                    kind='plain'
                    size='small'
                    onClick={() => setShowEditSpendLimit(true)}
                  >
                    {getLocale('braveWalletEditPermissionsTitle')}
                  </Button>
                </Row>

                {/* Network info box */}
                <InfoBox
                  padding='16px'
                  gap='8px'
                  width='100%'
                >
                  <ConfirmationNetworkFee
                    transactionsNetwork={network}
                    gasFee={gasFee}
                    transactionDetails={transactionDetails}
                    onClickEditNetworkFee={onClickEditNetworkFee}
                  />
                </InfoBox>

                {/* Approval target info box */}
                <InfoBox
                  padding='16px'
                  gap='8px'
                  width='100%'
                >
                  {/* Approval target */}
                  <Row justifyContent='space-between'>
                    <ConfirmationInfoLabel
                      textColor='primary'
                      textAlign='left'
                    >
                      {getLocale('braveWalletApprovalTarget')}
                    </ConfirmationInfoLabel>
                    <Row
                      width='unset'
                      gap='4px'
                    >
                      <Tooltip text={transactionDetails.approvalTarget}>
                        <ConfirmationButtonLink
                          onClick={onClickViewOnBlockExplorer(
                            'contract',
                            transactionDetails?.approvalTarget ?? '',
                          )}
                        >
                          {transactionDetails.approvalTargetLabel}
                          <Icon name='arrow-diagonal-up-right' />
                        </ConfirmationButtonLink>
                      </Tooltip>
                    </Row>
                  </Row>

                  {/* Current approval limit */}
                  <Row justifyContent='space-between'>
                    <ConfirmationInfoLabel
                      textColor='primary'
                      textAlign='left'
                    >
                      {getLocale('braveWalletCurrentApprovalLimit')}
                    </ConfirmationInfoLabel>
                    {currentLimit && (
                      <AmountText
                        textColor='primary'
                        textAlign='right'
                      >
                        {isCurrentAllowanceUnlimited
                          ? getLocale('braveWalletTransactionApproveUnlimited')
                            + ' '
                            + (token?.symbol || '')
                          : new Amount(currentLimit).formatAsAsset(
                              undefined,
                              token?.symbol ?? '',
                            )}
                      </AmountText>
                    )}
                  </Row>

                  {/* Proposed approval limit */}
                  <Row justifyContent='space-between'>
                    <ConfirmationInfoLabel
                      textColor='primary'
                      textAlign='left'
                    >
                      {getLocale('braveWalletProposedApprovalLimit')}
                    </ConfirmationInfoLabel>
                    <AmountText
                      textColor={isApprovalUnlimited ? 'error' : 'primary'}
                      textAlign='right'
                    >
                      {isApprovalUnlimited
                        ? getLocale('braveWalletTransactionApproveUnlimited')
                          + ' '
                          + (token?.symbol || '')
                        : new Amount(
                            transactionDetails.valueExact,
                          ).formatAsAsset(2, token?.symbol || '')}
                    </AmountText>
                  </Row>
                </InfoBox>

                {/* Advanced settings and details buttons */}
                <ConfirmationFooterActions
                  onClickAdvancedSettings={onClickAdvancedSettings}
                  onClickDetails={onClickDetails}
                />
              </Column>
            </Column>

            {/* Reject and confirm buttons */}
            <Row
              padding='16px'
              gap='8px'
            >
              <Button
                kind='outline'
                size='medium'
                onClick={onReject}
              >
                {getLocale('braveWalletAllowSpendRejectButton')}
              </Button>
              <Button
                kind='filled'
                size='medium'
                onClick={onConfirm}
              >
                {getLocale('braveWalletAllowSpendConfirmButton')}
              </Button>
            </Row>
          </Card>
        </Column>
      </StyledWrapper>

      {/* Edit spend limit */}
      <BottomSheet
        isOpen={showEditSpendLimit}
        title={getLocale('braveWalletEditPermissionsTitle')}
        onClose={() => setShowEditSpendLimit(false)}
      >
        <EditSpendLimit
          onCancel={() => setShowEditSpendLimit(false)}
          onSave={onSaveSpendLimit}
          approvalTarget={transactionDetails?.approvalTargetLabel ?? ''}
          isApprovalUnlimited={transactionDetails?.isApprovalUnlimited ?? false}
          proposedAllowance={transactionDetails.valueExact}
          symbol={token?.symbol ?? ''}
        />
      </BottomSheet>
    </>
  )
}

export default AllowSpendPanel
