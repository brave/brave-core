// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Tooltip from '@brave/leo/react/tooltip'

// Utils
import { getLocale } from '../../../../common/locale'

// Hooks
import {
  usePendingTransactions, //
} from '../../../common/hooks/use-pending-transaction'

// Components
import {
  ConfirmationTokenInfo, //
} from '../confirmation_token_info/confirmation_token_info'
import { ConfirmationHeader } from '../confirmation_header/confirmation_header'
import { LoadingPanel } from '../loading_panel/loading_panel'
import { EditNetworkFee } from '../edit_network_fee/edit_network_fee'
import {
  CreateAccountIcon, //
} from '../../shared/create-account-icon/create-account-icon'
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
  PriorityLabel,
  FeeButton,
  InfoText,
  InfoIcon,
} from './cancel_speedup_transaction.style'
import { Column, Row } from '../../shared/style'

export function CancelSpeedupTransaction() {
  // State
  const [showEditNetworkFee, setShowEditNetworkFee] =
    React.useState<boolean>(false)

  // Hooks
  const {
    transactionDetails,
    selectedPendingTransaction,
    queueNextTransaction,
    queuePreviousTransaction,
    gasFee,
    transactionsQueueLength,
    rejectAllTransactions,
    onConfirm,
    onReject,
    transactionsNetwork,
    fromAccount,
    isConfirmButtonDisabled,
    isShieldingFunds,
    insufficientFundsForGasError,
    insufficientFundsError,
  } = usePendingTransactions()

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
          title={getLocale('braveWalletTransactionCancel')}
          transactionsQueueLength={transactionsQueueLength}
          queueNextTransaction={queueNextTransaction}
          queuePreviousTransaction={queuePreviousTransaction}
          rejectAllTransactions={rejectAllTransactions}
        />
        <Column
          width='100%'
          height='100%'
          justifyContent='flex-start'
          padding='0px 16px'
          gap='16px'
        >
          <CreateAccountIcon
            size='extra-huge'
            account={fromAccount}
          />
          <Title textColor='primary'>
            {getLocale('braveWalletPanelTitle')}
          </Title>

          {/* Transaction details */}
          <Card width='100%'>
            <Column
              width='100%'
              padding='16px'
            >
              <ConfirmationTokenInfo
                token={transactionDetails.token}
                label='fee'
                amount={gasFee}
                fiatValue={transactionDetails.gasFeeFiat}
                network={transactionsNetwork}
                account={fromAccount}
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
          </Card>

          {/* Network fee actions */}
          <Card
            width='100%'
            padding='16px'
          >
            <Row justifyContent='space-between'>
              <PriorityLabel textColor='primary'>
                {getLocale('braveWalletSpeedPriority')}
              </PriorityLabel>
              <FeeButton onClick={() => setShowEditNetworkFee(true)}>
                {getLocale('braveWalletAllowSpendEditButton')}
              </FeeButton>
            </Row>
          </Card>

          <Row margin='4px 0px 0px 0px'>
            <Tooltip mode='default'>
              <Row
                maxWidth='300px'
                slot='content'
              >
                {getLocale('braveWalletGasFeeTooltipDescription')}
              </Row>
              <Row gap='8px'>
                <InfoText textColor='tertiary'>
                  {getLocale('braveWalletGasFeeTooltip')}
                </InfoText>
                <InfoIcon name='info-outline' />
              </Row>
            </Tooltip>
          </Row>
        </Column>

        {/* Confirm and reject buttons */}
        <ConfirmRejectButtons
          onConfirm={onConfirm}
          onReject={onReject}
          isConfirmButtonDisabled={isConfirmButtonDisabled}
          isShieldingFunds={isShieldingFunds}
        />
      </StyledWrapper>

      {/* Edit network fee */}
      <EditNetworkFee
        isOpen={showEditNetworkFee}
        onCancel={() => setShowEditNetworkFee(false)}
      />
    </>
  )
}
