// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Mock Data
import {
  mockMeldCryptoCurrencies //
} from '../../../../../common/constants/mocks'

// Components
import { SelectAssetButton } from './select_asset_button'
import {
  WalletPageStory //
} from '../../../../../stories/wrappers/wallet-page-story-wrapper'

export const _SelectAssetButton = () => {
  return (
    <WalletPageStory>
      <SelectAssetButton
        labelText='Asset'
        selectedAsset={mockMeldCryptoCurrencies[0]}
        onClick={() => console.log('Open asset selection modal')}
      />
    </WalletPageStory>
  )
}

export default {
  component: _SelectAssetButton,
  title: 'Fund Wallet - Select Asset Button'
}
