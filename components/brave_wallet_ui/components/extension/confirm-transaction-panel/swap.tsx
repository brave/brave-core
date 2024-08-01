// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { getLocale } from '../../../../common/locale'
import { isBridgeTransaction } from '../../../utils/tx-utils'

// Styled components
import { HeaderTitle } from './swap.style'
import { NetworkText, StyledWrapper, TopRow } from './style'
import { Origin } from './common/origin'
import { EditPendingTransactionGas } from './common/gas'

// Components
import { TransactionQueueSteps } from './common/queue'
import {
  PendingTransactionActionsFooter //
} from './common/pending_tx_actions_footer'
import AdvancedTransactionSettings from '../advanced-transaction-settings'
import {
  PendingTransactionNetworkFeeAndSettings //
} from '../pending-transaction-network-fee/pending-transaction-network-fee'
import { SwapBase } from '../swap'

// Hooks
import { usePendingTransactions } from '../../../common/hooks/use-pending-transaction'
import { useSwapTransactionParser } from '../../../common/hooks/use-swap-tx-parser'
import { useGetActiveOriginQuery } from '../../../common/slices/api.slice'

export function ConfirmSwapTransaction() {
  // state
  const [showAdvancedTransactionSettings, setShowAdvancedTransactionSettings] =
    React.useState<boolean>(false)
  const [isEditingGas, setIsEditingGas] = React.useState<boolean>(false)
  const [isWarningCollapsed, setIsWarningCollapsed] = React.useState(true)

  // hooks
  const {
    transactionDetails,
    fromOrb,
    toOrb,
    updateUnapprovedTransactionNonce,
    selectedPendingTransaction,
    onConfirm,
    onReject,
    queueNextTransaction,
    transactionQueueNumber,
    transactionsQueueLength,
    rejectAllTransactions,
    isConfirmButtonDisabled,
    insufficientFundsError,
    insufficientFundsForGasError
  } = usePendingTransactions()

  // queries
  const { data: activeOrigin = { eTldPlusOne: '', originSpec: '' } } =
    useGetActiveOriginQuery()

  // computed
  const originInfo = selectedPendingTransaction?.originInfo ?? activeOrigin
  const isBridgeTx = selectedPendingTransaction
    ? isBridgeTransaction(selectedPendingTransaction)
    : false

  // Methods
  const onToggleAdvancedTransactionSettings = () => {
    setShowAdvancedTransactionSettings(!showAdvancedTransactionSettings)
  }
  const onToggleEditGas = () => setIsEditingGas(!isEditingGas)

  const { buyToken, sellToken, buyAmountWei, sellAmountWei } =
    useSwapTransactionParser(selectedPendingTransaction)

  // render
  if (
    showAdvancedTransactionSettings &&
    transactionDetails &&
    selectedPendingTransaction
  ) {
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

  if (isEditingGas) {
    return <EditPendingTransactionGas onCancel={onToggleEditGas} />
  }

  return (
    <StyledWrapper>
      <TopRow>
        <NetworkText />
        <TransactionQueueSteps
          queueNextTransaction={queueNextTransaction}
          transactionQueueNumber={transactionQueueNumber}
          transactionsQueueLength={transactionsQueueLength}
        />
      </TopRow>

      <HeaderTitle>{getLocale('braveWalletSwapReviewHeader')}</HeaderTitle>

      <Origin originInfo={originInfo} />

      {isWarningCollapsed && (
        <SwapBase
          sellToken={sellToken}
          buyToken={buyToken}
          sellAmount={
            !sellAmountWei.isUndefined() ? sellAmountWei.format() : undefined
          }
          buyAmount={
            !buyAmountWei.isUndefined() ? buyAmountWei.format() : undefined
          }
          senderLabel={transactionDetails?.senderLabel}
          senderOrb={fromOrb}
          recipientOrb={toOrb}
          recipientLabel={transactionDetails?.recipientLabel}
          // set to true once Swap+Send is supported
          expectRecipientAddress={false}
          isBridgeTx={isBridgeTx}
          toChainId={selectedPendingTransaction?.swapInfo?.toChainId}
          toCoin={selectedPendingTransaction?.swapInfo?.toCoin}
        />
      )}

      <PendingTransactionNetworkFeeAndSettings
        onToggleAdvancedTransactionSettings={
          onToggleAdvancedTransactionSettings
        }
        onToggleEditGas={onToggleEditGas}
      />

      <PendingTransactionActionsFooter
        onConfirm={onConfirm}
        onReject={onReject}
        isConfirmButtonDisabled={isConfirmButtonDisabled}
        rejectAllTransactions={rejectAllTransactions}
        transactionDetails={transactionDetails}
        transactionsQueueLength={transactionsQueueLength}
        insufficientFundsForGasError={insufficientFundsForGasError}
        insufficientFundsError={insufficientFundsError}
        isWarningCollapsed={isWarningCollapsed}
        setIsWarningCollapsed={setIsWarningCollapsed}
      />
    </StyledWrapper>
  )
}
