// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Components
import {
  WalletPageStory //
} from '../../../../stories/wrappers/wallet-page-story-wrapper'
import { Web3DappFilters } from './web3_dapp_filters_modal'

const dappCategories = [
  'Games',
  'DeFi',
  'Collectibles',
  'Gambling',
  'Social',
  'Exchanges'
]

export const _Web3DappFilters = {
  render: () => {
    return (
      <WalletPageStory>
        <Web3DappFilters
          categories={dappCategories}
          onClose={() => {}}
        />
      </WalletPageStory>
    )
  }
}

export default {
  component: Web3DappFilters
}
