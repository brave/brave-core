// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// components
import {
  WalletPanelStory //
} from '../../../../../../stories/wrappers/wallet-panel-story-wrapper'
import { PanelWrapper } from '../../../../../../panel/style'
import { NFTGridViewItem } from './nft-grid-view-item'

// mocks
import {
  mockMoonCatNFT //
} from '../../../../../../stories/mock-data/mock-asset-options'

export const _NftGridViewItem = {
  render: () => {
    return (
      <WalletPanelStory>
        <PanelWrapper>
          <NFTGridViewItem
            isTokenHidden={false}
            isTokenSpam={false}
            onSelectAsset={() => undefined}
            token={mockMoonCatNFT}
            isWatchOnly={false}
          />
        </PanelWrapper>
      </WalletPanelStory>
    )
  }
}
export const _JunkAndWatchOnlyNftGridViewItem = {
  render: () => {
    return (
      <WalletPanelStory>
        <PanelWrapper>
          <NFTGridViewItem
            isTokenHidden={false}
            isTokenSpam={true}
            onSelectAsset={() => undefined}
            token={mockMoonCatNFT}
            isWatchOnly={true}
          />
        </PanelWrapper>
      </WalletPanelStory>
    )
  }
}
export default {
  component: NFTGridViewItem
}
