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

// Queries
import {
  useGetDefaultFiatCurrencyQuery, //
} from '../../../common/slices/api.slice'

// Utils
import { getLocale, formatLocale } from '../../../../common/locale'
import Amount from '../../../utils/amount'

// Types
import { BraveWallet } from '../../../constants/types'
import { ParsedTransaction } from '../../../utils/tx-utils'

// Components
import { OriginInfoCard } from '../origin_info_card/origin_info_card'
import {
  TransactionQueueSelector, //
} from '../transaction_queue_selector/transaction_queue_selector'
import { EditSpendLimit } from '../edit_spend_permissions/edit_spend_limit'
import { BottomSheet } from '../../shared/bottom_sheet/bottom_sheet'

// Styled Components
import {
  StyledWrapper,
  HeaderText,
  Card,
  Title,
  Description,
  InfoBox,
  InfoLabel,
  InfoText,
  AmountText,
  ButtonLink,
  TokenButtonLink,
} from './allow_spend_panel.style'
import {
  Column,
  HorizontalSpace,
  Row,
  VerticalDivider,
} from '../../shared/style'

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

  // Queries
  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()

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
        <Row
          padding='18px'
          justifyContent={
            transactionsQueueLength > 1 ? 'space-between' : 'center'
          }
        >
          {transactionsQueueLength > 1 && <HorizontalSpace space='110px' />}
          <HeaderText textColor='primary'>
            {getLocale('braveWalletSpendLimit')}
          </HeaderText>
          <TransactionQueueSelector
            transactionsQueueLength={transactionsQueueLength}
            queueNextTransaction={queueNextTransaction}
            queuePreviousTransaction={queuePreviousTransaction}
            rejectAllTransactions={rejectAllTransactions}
          />
        </Row>
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
                  <Row justifyContent='space-between'>
                    <InfoLabel textColor='secondary'>
                      {getLocale('braveWalletTransactionDetailNetwork')}
                    </InfoLabel>
                    <InfoLabel textColor='primary'>
                      {network?.chainName ?? ''}
                    </InfoLabel>
                  </Row>
                  <VerticalDivider />
                  <Row
                    justifyContent='space-between'
                    alignItems='flex-start'
                  >
                    <Column
                      alignItems='flex-start'
                      justifyContent='flex-start'
                      gap='4px'
                    >
                      <InfoLabel
                        textColor='secondary'
                        textAlign='left'
                      >
                        {getLocale('braveWalletAllowSpendTransactionFee')}
                      </InfoLabel>
                      <Button
                        size='tiny'
                        kind='plain'
                        onClick={onClickEditNetworkFee}
                      >
                        <Icon
                          name='tune'
                          slot='icon-before'
                        />
                        {getLocale('braveWalletAllowSpendEditButton')}
                      </Button>
                    </Column>
                    <Column
                      alignItems='flex-end'
                      justifyContent='flex-start'
                      gap='8px'
                    >
                      <InfoLabel
                        textColor='primary'
                        textAlign='right'
                      >
                        {(network
                          && new Amount(gasFee)
                            .divideByDecimals(network.decimals)
                            .formatAsAsset(6, network.symbol))
                          || ''}
                      </InfoLabel>
                      <InfoText
                        textColor='tertiary'
                        textAlign='right'
                      >
                        {new Amount(transactionDetails.gasFeeFiat).formatAsFiat(
                          defaultFiatCurrency,
                        )}
                      </InfoText>
                    </Column>
                  </Row>
                </InfoBox>

                {/* Approval target info box */}
                <InfoBox
                  padding='16px'
                  gap='8px'
                  width='100%'
                >
                  {/* Approval target */}
                  <Row justifyContent='space-between'>
                    <InfoLabel
                      textColor='primary'
                      textAlign='left'
                    >
                      {getLocale('braveWalletApprovalTarget')}
                    </InfoLabel>
                    <Row
                      width='unset'
                      gap='4px'
                    >
                      <Tooltip text={transactionDetails.approvalTarget}>
                        <ButtonLink
                          onClick={onClickViewOnBlockExplorer(
                            'contract',
                            transactionDetails?.approvalTarget ?? '',
                          )}
                        >
                          {transactionDetails.approvalTargetLabel}
                          <Icon name='arrow-diagonal-up-right' />
                        </ButtonLink>
                      </Tooltip>
                    </Row>
                  </Row>

                  {/* Current approval limit */}
                  <Row justifyContent='space-between'>
                    <InfoLabel
                      textColor='primary'
                      textAlign='left'
                    >
                      {getLocale('braveWalletCurrentApprovalLimit')}
                    </InfoLabel>
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
                    <InfoLabel
                      textColor='primary'
                      textAlign='left'
                    >
                      {getLocale('braveWalletProposedApprovalLimit')}
                    </InfoLabel>
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
                <Row justifyContent='space-between'>
                  <div>
                    <Button
                      kind='plain'
                      size='tiny'
                      onClick={onClickAdvancedSettings}
                    >
                      <Icon
                        name='settings'
                        slot='icon-before'
                      />
                      {getLocale('braveWalletAdvancedTransactionSettings')}
                    </Button>
                  </div>
                  <div>
                    <Button
                      kind='plain'
                      size='tiny'
                      onClick={onClickDetails}
                    >
                      <Icon
                        name='info-outline'
                        slot='icon-before'
                      />
                      {getLocale('braveWalletDetails')}
                    </Button>
                  </div>
                </Row>
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
