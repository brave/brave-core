// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Tooltip from '@brave/leo/react/tooltip'
import Icon from '@brave/leo/react/icon'

// Queries
import { useGetActiveOriginQuery } from '../../../common/slices/api.slice'

// Hooks
import { useExplorer } from '../../../common/hooks/explorer'
import {
  usePendingTransactions, //
} from '../../../common/hooks/use-pending-transaction'

// Utils
import { getLocale, formatLocale } from '../../../../common/locale'
import Amount from '../../../utils/amount'

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
import {
  PendingTransactionDetails, //
} from '../pending_transaction_details/pending_transaction_details'
import { EditNetworkFee } from '../edit_network_fee/edit_network_fee'
import {
  AdvancedTransactionSettings, //
} from '../advanced_transaction_settings/advanced_transaction_settings'
import { LoadingPanel } from '../loading_panel/loading_panel'
import {
  ConfirmRejectButtons, //
} from '../confirm_reject_buttons/confirm_reject_buttons'
import {
  ConfirmationError, //
} from '../confirmation_error/confirmation_error'

// Styled Components
import {
  StyledWrapper,
  Card,
  Title,
  Description,
  InfoBox,
  AmountText,
  TokenButtonLink,
  ContentWrapper,
} from './allow_spend_panel.style'
import { Column, Row, VerticalDivider } from '../../shared/style'
import {
  ConfirmationInfoLabel,
  ConfirmationButtonLink,
  ScrollableColumn,
} from '../shared-panel-styles'

export const AllowSpendPanel = () => {
  // Queries
  const { data: activeOrigin = { eTldPlusOne: '', originSpec: '' } } =
    useGetActiveOriginQuery()

  // State
  const [showEditSpendLimit, setShowEditSpendLimit] = React.useState(false)
  const [showEditNetworkFee, setShowEditNetworkFee] =
    React.useState<boolean>(false)
  const [showAdvancedTransactionSettings, setShowAdvancedTransactionSettings] =
    React.useState<boolean>(false)
  const [showTransactionDetails, setShowTransactionDetails] =
    React.useState<boolean>(false)

  // Hooks
  const {
    erc20ApproveTokenInfo,
    onEditAllowanceSave,
    transactionDetails,
    transactionsNetwork,
    updateUnapprovedTransactionNonce,
    isCurrentAllowanceUnlimited,
    currentTokenAllowance,
    selectedPendingTransaction,
    onConfirm,
    onReject,
    gasFee,
    queueNextTransaction,
    queuePreviousTransaction,
    transactionsQueueLength,
    rejectAllTransactions,
    canEditNetworkFee,
    isConfirmButtonDisabled,
    insufficientFundsError,
    insufficientFundsForGasError,
    fromAccount,
  } = usePendingTransactions()

  const onClickViewOnBlockExplorer = useExplorer(transactionsNetwork)

  // Computed
  const isApprovalUnlimited = transactionDetails?.isApprovalUnlimited ?? false
  const originInfo = selectedPendingTransaction?.originInfo ?? activeOrigin

  if (!selectedPendingTransaction || !transactionDetails) {
    return <LoadingPanel />
  }

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
        <ContentWrapper
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
            {originInfo && <OriginInfoCard origin={originInfo} />}
            <ScrollableColumn
              width='100%'
              height='100%'
              justifyContent='flex-start'
            >
              <Column
                width='100%'
                padding='16px 16px 0px 16px'
                gap='8px'
                justifyContent='flex-start'
              >
                <Title>
                  {formatLocale('braveWalletAllowSpendTitle', {
                    $1: (
                      <Row
                        width='unset'
                        margin='0px 0px 0px 4px'
                      >
                        <Tooltip
                          text={erc20ApproveTokenInfo?.contractAddress ?? ''}
                        >
                          <TokenButtonLink
                            onClick={onClickViewOnBlockExplorer(
                              'token',
                              erc20ApproveTokenInfo?.contractAddress ?? '',
                            )}
                          >
                            {erc20ApproveTokenInfo?.symbol ?? ''}
                          </TokenButtonLink>
                        </Tooltip>
                      </Row>
                    ),
                  })}
                </Title>
                <Description textColor='tertiary'>
                  {getLocale('braveWalletAllowSpendDescription').replace(
                    '$1',
                    erc20ApproveTokenInfo?.symbol ?? '',
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
                <InfoBox width='100%'>
                  <Column
                    padding={
                      isConfirmButtonDisabled ? '16px 16px 8px 16px' : '16px'
                    }
                    gap='8px'
                    width='100%'
                  >
                    <ConfirmationNetworkFee
                      transactionsNetwork={transactionsNetwork}
                      gasFee={gasFee}
                      transactionDetails={transactionDetails}
                      onClickEditNetworkFee={
                        canEditNetworkFee
                          ? () => setShowEditNetworkFee(true)
                          : undefined
                      }
                    />
                  </Column>

                  {/* Transaction errors */}
                  <ConfirmationError
                    insufficientFundsError={insufficientFundsError}
                    insufficientFundsForGasError={insufficientFundsForGasError}
                    transactionDetails={transactionDetails}
                    transactionsNetwork={transactionsNetwork}
                    account={fromAccount}
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
                    {currentTokenAllowance && (
                      <AmountText
                        textColor='primary'
                        textAlign='right'
                      >
                        {isCurrentAllowanceUnlimited
                          ? getLocale('braveWalletTransactionApproveUnlimited')
                            + ' '
                            + (erc20ApproveTokenInfo?.symbol || '')
                          : new Amount(currentTokenAllowance).formatAsAsset(
                              undefined,
                              erc20ApproveTokenInfo?.symbol ?? '',
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
                          + (erc20ApproveTokenInfo?.symbol || '')
                        : new Amount(
                            transactionDetails.valueExact,
                          ).formatAsAsset(
                            2,
                            erc20ApproveTokenInfo?.symbol || '',
                          )}
                    </AmountText>
                  </Row>
                </InfoBox>

                {/* Advanced settings and details buttons */}
                <ConfirmationFooterActions
                  onClickAdvancedSettings={() =>
                    setShowAdvancedTransactionSettings(true)
                  }
                  onClickDetails={() => setShowTransactionDetails(true)}
                />
              </Column>
            </ScrollableColumn>

            {/* Confirm and reject buttons */}
            <ConfirmRejectButtons
              onConfirm={onConfirm}
              onReject={onReject}
              isConfirmButtonDisabled={isConfirmButtonDisabled}
            />
          </Card>
        </ContentWrapper>
      </StyledWrapper>

      {/* Edit spend limit */}
      <BottomSheet
        isOpen={showEditSpendLimit}
        title={getLocale('braveWalletEditPermissionsTitle')}
        onClose={() => setShowEditSpendLimit(false)}
      >
        <EditSpendLimit
          onCancel={() => setShowEditSpendLimit(false)}
          onSave={onEditAllowanceSave}
          approvalTarget={transactionDetails?.approvalTargetLabel ?? ''}
          isApprovalUnlimited={transactionDetails?.isApprovalUnlimited ?? false}
          proposedAllowance={transactionDetails.valueExact}
          symbol={erc20ApproveTokenInfo?.symbol ?? ''}
        />
      </BottomSheet>

      {/* Transaction details */}
      <BottomSheet
        isOpen={showTransactionDetails}
        title={getLocale('braveWalletDetails')}
        onClose={() => setShowTransactionDetails(false)}
      >
        <PendingTransactionDetails
          transactionInfo={selectedPendingTransaction}
          instructions={transactionDetails.instructions}
        />
      </BottomSheet>

      {/* Advanced transaction settings */}
      <BottomSheet
        isOpen={showAdvancedTransactionSettings}
        title={getLocale('braveWalletAdvancedTransactionSettings')}
        onClose={() => setShowAdvancedTransactionSettings(false)}
      >
        <AdvancedTransactionSettings
          onCancel={() => setShowAdvancedTransactionSettings(false)}
          nonce={transactionDetails.nonce}
          onSave={(nonce: string) =>
            updateUnapprovedTransactionNonce({
              chainId: selectedPendingTransaction.chainId,
              txMetaId: selectedPendingTransaction.id,
              nonce: nonce,
            })
          }
        />
      </BottomSheet>

      {/* Edit network fee */}
      <EditNetworkFee
        isOpen={showEditNetworkFee}
        onCancel={() => setShowEditNetworkFee(false)}
      />
    </>
  )
}

export default AllowSpendPanel
