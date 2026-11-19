// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import {
  mockETHSwapTransaction,
  mockETHTokenSwapWithNativeValueTransaction,
} from '../../../stories/mock-data/mock-transaction-info'

// Components
import { ConfirmSwapTransaction } from './confirm_swap_transaction'
import {
  WalletPanelStory, //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'

const renderForTransaction = (id: string) => (
  <WalletPanelStory uiStateOverride={{ selectedPendingTransactionId: id }}>
    <ConfirmSwapTransaction />
  </WalletPanelStory>
)

export const _ConfirmSwapTransaction = {
  render: () => renderForTransaction(mockETHSwapTransaction.id),
}

export const _ConfirmSwapTransactionWithNativeValue = {
  render: () =>
    renderForTransaction(mockETHTokenSwapWithNativeValueTransaction.id),
}

export default {
  title: 'Wallet/Panel/Panels/Confirm Swap Transaction Panel',
  component: ConfirmSwapTransaction,
  parameters: {
    layout: 'centered',
  },
}
