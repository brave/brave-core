// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { AllowAddChangeNetworkPanel } from '.'
import { PanelWrapper } from '../../../panel/style'

import { mockNetworks } from '../../../stories/mock-data/mock-networks'
import { mockOriginInfo } from '../../../stories/mock-data/mock-origin-info'
import WalletPanelStory from '../../../stories/wrappers/wallet-panel-story-wrapper'

export const _AllowAddChangeNetwork = () => {
  return (
    <WalletPanelStory>
      <PanelWrapper>
        <AllowAddChangeNetworkPanel
          originInfo={mockOriginInfo}
          panelType='change'
          networkPayload={mockNetworks[0]}
        />
      </PanelWrapper>
    </WalletPanelStory>
  )
}

_AllowAddChangeNetwork.storyName = 'Allow Add or Change Network'

export default _AllowAddChangeNetwork
