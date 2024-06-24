// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// components
import { AllowAddChangeNetworkPanel } from '.'
import { PanelWrapper } from '../../../panel/style'
import {
  WalletPanelStory //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'

// mocks
import { mockSwitchChainRequest } from '../../../stories/mock-data/mock-eth-requests'

export const _AllowAddChangeNetwork = {
  render: () => {
    return (
      <WalletPanelStory>
        <PanelWrapper>
          <AllowAddChangeNetworkPanel
            switchChainRequest={mockSwitchChainRequest}
          />
        </PanelWrapper>
      </WalletPanelStory>
    )
  }
}

export default { component: AllowAddChangeNetworkPanel }
