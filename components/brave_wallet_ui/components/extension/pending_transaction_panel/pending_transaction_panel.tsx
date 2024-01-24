// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import {
  BraveWallet,
  SerializableTransactionInfo
} from '../../../constants/types'

// Components
import { ConfirmSwapTransaction } from '../confirm-transaction-panel/swap'
import {
  ConfirmTransactionPanel //
} from '../confirm-transaction-panel/confirm-transaction-panel'
import { LoadingPanel } from '../loading_panel/loading_panel'

interface Props {
  selectedPendingTransaction: SerializableTransactionInfo
}

export const PendingTransactionPanel: React.FC<Props> = ({
  selectedPendingTransaction
}) => {
  // render

  // Loading/Fetching
  if (!selectedPendingTransaction) {
    return <LoadingPanel />
  }

  // Safer-Sign (Brave Swap)
  if (
    selectedPendingTransaction.txType === BraveWallet.TransactionType.ETHSwap
  ) {
    return <ConfirmSwapTransaction />
  }

  // Defaults
  return <ConfirmTransactionPanel />
}
