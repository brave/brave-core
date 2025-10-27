// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import {
  mockETHNativeTokenSendTransaction, //
} from '../../../stories/mock-data/mock-transaction-info'

// Components
import { CancelSpeedupTransaction } from './cancel_speedup_transaction'
import {
  WalletPanelStory, //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'

export const _ConfirmSendTransaction = {
  render: () => {
    return <CancelSpeedupTransaction />
  },
}

export default {
  title: 'Wallet/Panel/Panels/Confirm Cancel Transaction Panel',
  component: CancelSpeedupTransaction,
  parameters: {
    layout: 'centered',
  },
  decorators: [
    (Story: any) => (
      <WalletPanelStory
        uiStateOverride={{
          selectedPendingTransactionId: mockETHNativeTokenSendTransaction.id,
        }}
      >
        <Story />
      </WalletPanelStory>
    ),
  ],
}
