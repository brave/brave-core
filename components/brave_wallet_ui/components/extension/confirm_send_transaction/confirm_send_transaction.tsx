// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'

// Constants
import { BraveWallet } from '../../../constants/types'

// Queries
import {
  useGetDefaultFiatCurrencyQuery, //
} from '../../../common/slices/api.slice'

// Utils
import { getLocale } from '../../../../common/locale'
import Amount from '../../../utils/amount'

// Hooks
import {
  usePendingTransactions, //
} from '../../../common/hooks/use-pending-transaction'

// Components
import {
  ConfirmationFooterActions, //
} from '../confirmation_footer_actions/confirmation_footer_actions'
import {
  ConfirmationNetworkFee, //
} from '../confirmation_network_fee/confirmation_network_fee'
import {
  ConfirmationTokenInfo, //
} from '../confirmation_token_info/confirmation_token_info'
import { ConfirmationHeader } from '../confirmation_header/confirmation_header'
import { LoadingPanel } from '../loading_panel/loading_panel'
import { BottomSheet } from '../../shared/bottom_sheet/bottom_sheet'
import {
  PendingTransactionDetails, //
} from '../pending_transaction_details/pending_transaction_details'
import {
  AdvancedTransactionSettings, //
} from '../advanced_transaction_settings/advanced_transaction_settings'
import { EditNetworkFee } from '../edit_network_fee/edit_network_fee'
import {
  CreateAccountIcon, //
} from '../../shared/create-account-icon/create-account-icon'

// Styled Components
import {
  StyledWrapper,
  InfoBox,
  Card,
  Title,
} from './confirm_send_transaction.style'
import { Column, Row, VerticalDivider } from '../../shared/style'
import {
  ConfirmationInfoLabel,
  ConfirmationInfoText,
} from '../shared-panel-styles'

export function ConfirmSendTransaction() {
  // State
  const [showEditNetworkFee, setShowEditNetworkFee] =
    React.useState<boolean>(false)
  const [showAdvancedTransactionSettings, setShowAdvancedTransactionSettings] =
    React.useState<boolean>(false)
  const [showTransactionDetails, setShowTransactionDetails] =
    React.useState<boolean>(false)

  // Hooks
  const {
    transactionDetails,
    selectedPendingTransaction,
    queueNextTransaction,
    queuePreviousTransaction,
    transactionsQueueLength,
    rejectAllTransactions,
    onConfirm,
    onReject,
    transactionsNetwork,
    gasFee,
    updateUnapprovedTransactionNonce,
    fromAccount,
    toAccount,
    canEditNetworkFee,
    isEthereumTransaction,
    isAssociatedTokenAccountCreation,
  } = usePendingTransactions()

  // Queries
  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()

  // Memos
  const totalAmount = React.useMemo(() => {
    if (
      !transactionDetails
      || !selectedPendingTransaction
      || !transactionsNetwork
    ) {
      return ''
    }
    if (
      selectedPendingTransaction.txType
        === BraveWallet.TransactionType.ERC20Transfer
      || selectedPendingTransaction.txType
        === BraveWallet.TransactionType.SolanaSPLTokenTransfer
      || selectedPendingTransaction.txType
        === BraveWallet.TransactionType
          .SolanaSPLTokenTransferWithAssociatedTokenAccountCreation
    ) {
      return (
        new Amount(transactionDetails.valueExact).formatAsAsset(
          undefined,
          transactionDetails.token?.symbol ?? '',
        )
        + ' + '
        + new Amount(gasFee)
          .divideByDecimals(transactionsNetwork.decimals)
          .formatAsAsset(6, transactionsNetwork.symbol)
      )
    }
    return new Amount(gasFee)
      .plus(transactionDetails.weiTransferredValue)
      .divideByDecimals(transactionsNetwork?.decimals ?? 18)
      .formatAsAsset(undefined, transactionsNetwork?.symbol ?? '')
  }, [
    selectedPendingTransaction,
    transactionDetails,
    transactionsNetwork,
    gasFee,
  ])

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
          title={getLocale('braveWalletConfirmSend')}
          transactionsQueueLength={transactionsQueueLength}
          queueNextTransaction={queueNextTransaction}
          queuePreviousTransaction={queuePreviousTransaction}
          rejectAllTransactions={rejectAllTransactions}
        />
        <Column
          width='100%'
          height='100%'
          padding='0px 16px'
          gap='12px'
          justifyContent='flex-start'
        >
          <CreateAccountIcon
            size='extra-huge'
            account={fromAccount}
          />
          <Title textColor='primary'>
            {getLocale('braveWalletPanelTitle')}
          </Title>
          <InfoBox width='100%'>
            {/* Swap details */}
            <Card
              width='100%'
              padding='16px'
            >
              {/* Send token and amount */}
              <ConfirmationTokenInfo
                token={transactionDetails.token}
                label='send'
                valueExact={transactionDetails.valueExact}
                fiatValue={transactionDetails.fiatValue}
                network={transactionsNetwork}
                account={fromAccount}
              />

              {/* Divider */}
              <Row
                justifyContent='space-between'
                padding='16px 0px'
              >
                <VerticalDivider />
              </Row>

              {/* Recipient info */}
              <ConfirmationTokenInfo
                label='to'
                receiveAddress={transactionDetails.recipient}
                isAssociatedTokenAccountCreation={
                  isAssociatedTokenAccountCreation
                }
                network={transactionsNetwork}
                account={toAccount}
              />
            </Card>

            {/* Network fee details */}
            <Column
              width='100%'
              padding='16px'
              gap='8px'
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
              <VerticalDivider />

              {/* Transaction total amount */}
              <Column width='100%'>
                <Row justifyContent='space-between'>
                  <ConfirmationInfoLabel
                    textColor='secondary'
                    textAlign='left'
                  >
                    {getLocale('braveWalletConfirmTransactionTotal')}
                  </ConfirmationInfoLabel>
                  <ConfirmationInfoLabel
                    textColor='primary'
                    textAlign='right'
                  >
                    {totalAmount}
                  </ConfirmationInfoLabel>
                </Row>
                <Row justifyContent='space-between'>
                  <ConfirmationInfoText
                    textColor='tertiary'
                    textAlign='left'
                  >
                    {getLocale('braveWalletConfirmTransactionAmountFee')}
                  </ConfirmationInfoText>
                  <ConfirmationInfoText
                    textColor='tertiary'
                    textAlign='right'
                  >
                    {new Amount(transactionDetails.fiatValue)
                      .plus(transactionDetails.gasFeeFiat)
                      .formatAsFiat(defaultFiatCurrency)}
                  </ConfirmationInfoText>
                </Row>
              </Column>
            </Column>
          </InfoBox>

          {/* Advanced settings and details buttons */}
          <ConfirmationFooterActions
            onClickAdvancedSettings={
              isEthereumTransaction
                ? () => setShowAdvancedTransactionSettings(true)
                : undefined
            }
            onClickDetails={() => {
              setShowTransactionDetails(true)
            }}
          />
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
      </StyledWrapper>

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
