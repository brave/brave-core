// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { WalletSelectors } from '../../../common/selectors'
import { getLocale } from '../../../../common/locale'

// Styled components
import { HeaderTitle } from './swap.style'
import { VerticalSpace } from '../../shared/style'
import { NetworkText, StyledWrapper, TopRow } from './style'

// Components
import { Origin } from './common/origin'
import { EditPendingTransactionGas } from './common/gas'
import { TransactionQueueSteps } from './common/queue'
import { Footer } from './common/footer'
import AdvancedTransactionSettings from '../advanced-transaction-settings'
import {
  PendingTransactionNetworkFeeAndSettings //
} from '../pending-transaction-network-fee/pending-transaction-network-fee'
import { SwapBase } from '../swap'
import {
  TxWarningBanner //
} from './common/tx_warning_banner'

// Hooks
import { usePendingTransactions } from '../../../common/hooks/use-pending-transaction'
import {
  useUnsafeWalletSelector //
} from '../../../common/hooks/use-safe-selector'

interface Props {
  retrySimulation?: () => void
}

export function ConfirmSwapTransaction({ retrySimulation }: Props) {
  // redux
  const activeOrigin = useUnsafeWalletSelector(WalletSelectors.activeOrigin)

  // state
  const [showAdvancedTransactionSettings, setShowAdvancedTransactionSettings] =
    React.useState<boolean>(false)
  const [isEditingGas, setIsEditingGas] = React.useState<boolean>(false)

  // hooks
  const {
    transactionDetails,
    fromOrb,
    toOrb,
    updateUnapprovedTransactionNonce,
    selectedPendingTransaction,
    queueNextTransaction,
    transactionQueueNumber,
    transactionsQueueLength
  } = usePendingTransactions()

  // computed
  const originInfo = selectedPendingTransaction?.originInfo ?? activeOrigin

  // Methods
  const onToggleAdvancedTransactionSettings = () => {
    setShowAdvancedTransactionSettings(!showAdvancedTransactionSettings)
  }
  const onToggleEditGas = () => setIsEditingGas(!isEditingGas)

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

      <SwapBase
        sellToken={transactionDetails?.sellToken}
        buyToken={transactionDetails?.buyToken}
        sellAmount={transactionDetails?.sellAmountWei?.format()}
        buyAmount={transactionDetails?.minBuyAmountWei?.format()}
        senderLabel={transactionDetails?.senderLabel}
        senderOrb={fromOrb}
        recipientOrb={toOrb}
        recipientLabel={transactionDetails?.recipientLabel}
        // set to true once Swap+Send is supported
        expectRecipientAddress={false}
      />

      <PendingTransactionNetworkFeeAndSettings
        onToggleAdvancedTransactionSettings={
          onToggleAdvancedTransactionSettings
        }
        onToggleEditGas={onToggleEditGas}
      />

      {retrySimulation ? (
        <>
          <VerticalSpace space='16px' />
          <TxWarningBanner retrySimulation={retrySimulation} />
        </>
      ) : null}

      <Footer rejectButtonType={'cancel'} />
    </StyledWrapper>
  )
}
