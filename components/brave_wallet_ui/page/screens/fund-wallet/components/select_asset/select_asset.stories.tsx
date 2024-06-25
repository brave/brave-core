// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { SelectAsset } from './select_asset'
import WalletPageStory from '../../../../../stories/wrappers/wallet-page-story-wrapper'
import {
  mockMeldCryptoCurrencies,
  mockMeldFiatCurrencies
} from '../../../../../common/constants/mocks'
import { MeldCryptoCurrency } from '../../../../../constants/types'

export const _SelectAsset = () => {
  const [selectedCurrency, setSelectedCurrency] = React.useState<
    MeldCryptoCurrency | undefined
  >(undefined)
  return (
    <WalletPageStory>
      <SelectAsset
        isOpen={true}
        assets={mockMeldCryptoCurrencies}
        isLoadingAssets={false}
        isLoadingSpotPrices={false}
        selectedAsset={selectedCurrency}
        selectedFiatCurrency={mockMeldFiatCurrencies[0]}
        onSelectAsset={(asset) => setSelectedCurrency(asset)}
      />
    </WalletPageStory>
  )
}

_SelectAsset.story = {
  name: 'Fund Wallet - Select Asset'
}

export default _SelectAsset
