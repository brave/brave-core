// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// components
import { AllowAddChangeNetworkPanel } from './allow_add_change_network_panel'
import {
  WalletPanelStory, //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'

// mocks
import {
  mockSwitchChainRequest,
  mockAddChainRequest,
} from '../../../stories/mock-data/mock-eth-requests'

export const _AllowSwitchNetwork = {
  render: () => {
    return (
      <AllowAddChangeNetworkPanel switchChainRequest={mockSwitchChainRequest} />
    )
  },
}

export const _AllowAddNetwork = {
  render: () => {
    return <AllowAddChangeNetworkPanel addChainRequest={mockAddChainRequest} />
  },
}

export default {
  title: 'Wallet/Panel/Panels/Network',
  component: AllowAddChangeNetworkPanel,
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
