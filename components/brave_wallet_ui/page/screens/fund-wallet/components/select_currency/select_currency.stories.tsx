// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Mock Data
import {
  mockMeldFiatCurrencies //
} from '../../../../../common/constants/mocks'

// Types
import { MeldFiatCurrency } from '../../../../../constants/types'

// Components
import { SelectCurrency } from './select_currency'
import {
  WalletPageStory //
} from '../../../../../stories/wrappers/wallet-page-story-wrapper'

export const _SelectCurrency = () => {
  // State
  const [selectedCurrency, setSelectedCurrency] = React.useState<
    MeldFiatCurrency | undefined
  >(undefined)

  return (
    <WalletPageStory>
      <SelectCurrency
        isOpen={true}
        onClose={() => alert('Close was clicked.')}
        currencies={mockMeldFiatCurrencies}
        selectedCurrency={selectedCurrency}
        onSelectCurrency={(currency) => setSelectedCurrency(currency)}
      />
    </WalletPageStory>
  )
}

export default {
  component: _SelectCurrency,
  title: 'Fund Wallet - Select Currency'
}
