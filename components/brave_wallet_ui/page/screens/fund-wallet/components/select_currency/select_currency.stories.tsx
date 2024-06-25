// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { SelectCurrency } from './select_currency'
import WalletPageStory from '../../../../../stories/wrappers/wallet-page-story-wrapper'
import { mockMeldFiatCurrencies } from '../../../../../common/constants/mocks'
import { MeldFiatCurrency } from '../../../../../constants/types'

export const _SelectCurrency = () => {
  const [selectedCurrency, setSelectedCurrency] = React.useState<
    MeldFiatCurrency | undefined
  >(undefined)
  return (
    <WalletPageStory>
      <SelectCurrency
        isOpen={true}
        currencies={mockMeldFiatCurrencies}
        selectedCurrency={selectedCurrency}
        onSelectCurrency={(currency) => setSelectedCurrency(currency)}
      />
    </WalletPageStory>
  )
}

_SelectCurrency.story = {
  name: 'Fund Wallet - Select Currency'
}

export default _SelectCurrency
