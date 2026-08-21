// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import { getIsBraveWalletOrigin } from '../../../utils/string-utils'

// selectors
import { UISelectors } from '../../../common/selectors'

// hooks
import {
  useSafeUISelector,
  useUnsafeUISelector,
} from '../../../common/hooks/use-safe-selector'
import {
  useSelectedPendingTransaction, //
} from '../../../common/hooks/use-pending-transaction'

// components
import { ConfirmationPopup } from '../../../components/extension/confirmation_popup/confirmation_popup'
import {
  PendingTransactionPanel, //
} from '../../../components/extension/pending_transaction_panel/pending_transaction_panel'
import { TransactionStatus } from '../../../components/extension/post-confirmation'

/**
 * In-page confirmation overlay for wallet-origin transactions on Desktop.
 * Mirrors the side-panel ConfirmationPopup pattern so Send/Swap/etc. can
 * confirm without opening the toolbar Wallet Panel.
 */
export function DesktopTransactionConfirmation() {
  // Selectors
  const isPanel = useSafeUISelector(UISelectors.isPanel)
  const isSidePanel = useSafeUISelector(UISelectors.isSidePanel)
  const isMobile = useSafeUISelector(UISelectors.isMobile)
  const selectedTransactionId = useUnsafeUISelector(
    UISelectors.selectedTransactionId,
  )
  const submittingTransaction = useUnsafeUISelector(
    UISelectors.submittingTransaction,
  )

  // Hooks
  const {
    selectedPendingTransaction,
    isLoading: isLoadingPendingTransactions,
  } = useSelectedPendingTransaction()

  // Computed
  const pendingOrConfirmingTransaction =
    selectedPendingTransaction ?? submittingTransaction

  // Desktop page / Side Panel only — not bubble Panel or mobile (Android/iOS).
  if (isMobile || (isPanel && !isSidePanel)) {
    return null
  }

  // selectedTransactionId is set by the in-page confirm flow (page store is
  // separate from the panel store), so post-confirm status is safe to show.
  if (selectedTransactionId && !submittingTransaction) {
    return (
      <ConfirmationPopup>
        <TransactionStatus transactionLookup={selectedTransactionId} />
      </ConfirmationPopup>
    )
  }

  if (
    pendingOrConfirmingTransaction?.originInfo
    && getIsBraveWalletOrigin(pendingOrConfirmingTransaction.originInfo)
  ) {
    return (
      <ConfirmationPopup isLoading={isLoadingPendingTransactions}>
        <PendingTransactionPanel
          selectedPendingTransaction={pendingOrConfirmingTransaction}
        />
      </ConfirmationPopup>
    )
  }

  return null
}
