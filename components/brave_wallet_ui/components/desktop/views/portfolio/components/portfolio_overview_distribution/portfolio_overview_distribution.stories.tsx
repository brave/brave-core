// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Mocks
import {
  mockBtcToken,
  mockEthToken,
} from '../../../../../../stories/mock-data/mock-asset-options'

// Components
import {
  WalletPageStory, //
} from '../../../../../../stories/wrappers/wallet-page-story-wrapper'
import { PortfolioOverviewDistribution } from './portfolio_overview_distribution'

export const _PortfolioOverviewDistribution = {
  render: () => {
    return (
      <WalletPageStory>
        <PortfolioOverviewDistribution
          data={[
            {
              kind: 'asset',
              token: mockBtcToken,
              value: 99,
              fiatValue: '99',
            },
            {
              kind: 'asset',
              token: mockEthToken,
              value: 1,
              fiatValue: '1',
            },
          ]}
        />
      </WalletPageStory>
    )
  },
}

export default {
  title: 'Wallet/Desktop/Components/Portfolio Overview Distribution',
  component: PortfolioOverviewDistribution,
}
