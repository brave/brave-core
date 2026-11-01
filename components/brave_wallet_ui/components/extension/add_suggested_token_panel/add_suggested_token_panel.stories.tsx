// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet } from '../../../constants/types'

// Components
import { AddSuggestedTokenPanel } from './add_suggested_token_panel'
import {
  WalletPanelStory, //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'

// Mock Data
import { mockUniswapOriginInfo } from '../../../stories/mock-data/mock-origin-info'
import { mockBasicAttentionToken } from '../../../stories/mock-data/mock-asset-options'

const mockSuggestToken: BraveWallet.AddSuggestTokenRequest = {
  dappOriginInfo: mockUniswapOriginInfo,
  token: mockBasicAttentionToken,
}

export const _AddSuggestedToken = {
  render: () => {
    return (
      <AddSuggestedTokenPanel
        suggestToken={mockSuggestToken}
        requestsLength={1}
      />
    )
  },
}

export default {
  title: 'Wallet/Panel/Panels',
  component: _AddSuggestedToken,
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
