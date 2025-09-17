// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Mocks
import {
  mockedErc20ApprovalTransaction, //
} from '../../../stories/mock-data/mock-transaction-info'

// Components
import { AllowSpendPanel } from './allow_spend_panel'
import {
  WalletPanelStory, //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'

export const _AllowSpendPanel = {
  render: () => {
    return <AllowSpendPanel />
  },
}

export default {
  title: 'Wallet/Panel/Panels/Confirm Allow Spend Panel',
  component: AllowSpendPanel,
  parameters: {
    layout: 'centered',
  },
  decorators: [
    (Story: any) => (
      <WalletPanelStory
        uiStateOverride={{
          selectedPendingTransactionId: mockedErc20ApprovalTransaction.id,
        }}
      >
        <Story />
      </WalletPanelStory>
    ),
  ],
}
