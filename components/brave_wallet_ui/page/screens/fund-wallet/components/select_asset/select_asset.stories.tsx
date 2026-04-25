// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Mock Data
import { mockMeldCryptoCurrencies } from '../../../../../common/constants/mocks'

// Types
import { MeldCryptoCurrency } from '../../../../../constants/types'

// Components
import { SelectAsset } from './select_asset'
import {
  WalletPageStory, //
} from '../../../../../stories/wrappers/wallet-page-story-wrapper'

export const _SelectAsset = () => {
  // State
  const [selectedCurrency, setSelectedCurrency] = React.useState<
    MeldCryptoCurrency | undefined
  >(undefined)

  return (
    <WalletPageStory>
      <SelectAsset
        isOpen={true}
        onClose={() => alert('Close was clicked.')}
        assets={mockMeldCryptoCurrencies}
        isLoadingAssets={false}
        selectedAsset={selectedCurrency}
        onSelectAsset={(asset) => setSelectedCurrency(asset)}
      />
    </WalletPageStory>
  )
}

export default {
  component: _SelectAsset,
  title: 'Wallet/Desktop/Components/Fund Wallet',
}
