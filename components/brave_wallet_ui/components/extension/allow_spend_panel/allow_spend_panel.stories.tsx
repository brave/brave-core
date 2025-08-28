// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Mocks
import {
  mockUniswapOriginInfo, //
} from '../../../stories/mock-data/mock-origin-info'
import {
  mockParsedERC20ApprovalTransaction, //
} from '../../../stories/mock-data/mock-transaction-info'
import {
  mockEthMainnet, //
} from '../../../stories/mock-data/mock-networks'
import {
  mockBasicAttentionToken, //
} from '../../../stories/mock-data/mock-asset-options'

// Components
import { AllowSpendPanel } from './allow_spend_panel'
import {
  WalletPanelStory, //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'

export const _AllowSpendPanel = {
  render: () => {
    return (
      <AllowSpendPanel
        token={mockBasicAttentionToken}
        transactionDetails={mockParsedERC20ApprovalTransaction}
        network={mockEthMainnet}
        originInfo={mockUniswapOriginInfo}
        currentLimit='1000'
        isCurrentAllowanceUnlimited={false}
        gasFee='3641000000'
        onSaveSpendLimit={() => {
          alert('Clicked save spend limit')
        }}
        onClickDetails={() => {
          alert('Clicked details')
        }}
        onClickAdvancedSettings={() => {
          alert('Clicked advanced settings')
        }}
        onConfirm={() => {
          alert('Clicked confirm')
        }}
        onReject={() => {
          alert('Clicked reject')
        }}
        onClickEditNetworkFee={() => {
          alert('Clicked edit network fee')
        }}
        transactionsQueueLength={1}
        queueNextTransaction={() => {}}
        queuePreviousTransaction={() => {}}
        rejectAllTransactions={() => {}}
      />
    )
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
      <WalletPanelStory>
        <Story />
      </WalletPanelStory>
    ),
  ],
}
