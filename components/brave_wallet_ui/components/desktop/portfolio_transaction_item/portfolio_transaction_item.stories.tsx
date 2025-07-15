// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// components
import {
  WalletPageStory, //
} from '../../../stories/wrappers/wallet-page-story-wrapper'
import WalletPageLayout from '../wallet-page-layout'
import WalletSubViewLayout from '../wallet-sub-view-layout'
import { PortfolioTransactionItem } from './portfolio_transaction_item'

// mocks
import {
  mockTransactionInfo, //
} from '../../../stories/mock-data/mock-transaction-info'

export const _PortfolioTransactionItem = {}

export default {
  title: 'Wallet/Desktop/Components/Transaction',
  render: () => (
    <WalletPageStory>
      <WalletPageLayout>
        <WalletSubViewLayout>
          <PortfolioTransactionItem transaction={mockTransactionInfo} />
        </WalletSubViewLayout>
      </WalletPageLayout>
    </WalletPageStory>
  ),
}
