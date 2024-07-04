// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { WalletPageStory } from '../../../../stories/wrappers/wallet-page-story-wrapper'
import { BalanceDetailsModal } from './balance_details_modal'
import { mockBtcToken } from '../../../../stories/mock-data/mock-asset-options'

const mockBitcoinBalances = {
  availableBalance: '1000000',
  pendingBalance: '0',
  totalBalance: '1000000'
}

export const _BalanceDetailsModal = {
  render: () => {
    return (
      <WalletPageStory>
        <BalanceDetailsModal
          onClose={() => { }}
          token={mockBtcToken}
          balances={mockBitcoinBalances}
        />
      </WalletPageStory>
    )
  }
}

export default { title: 'Balance Details' }
