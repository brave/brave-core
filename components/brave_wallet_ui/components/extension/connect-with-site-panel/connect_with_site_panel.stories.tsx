// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Mocks
import {
  mockUniswapOriginInfo, //
} from '../../../stories/mock-data/mock-origin-info'
import { mockAccount } from '../../../common/constants/mocks'

// Components
import { ConnectWithSite } from './connect-with-site-panel'
import {
  WalletPanelStory, //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'

export const _ConnectWithSitePanel = {
  render: () => {
    return (
      <ConnectWithSite
        accountsToConnect={[mockAccount]}
        originInfo={mockUniswapOriginInfo}
      />
    )
  },
}

export default {
  title: 'Wallet/Panel/Panels/Connect With Site Panel',
  component: ConnectWithSite,
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
