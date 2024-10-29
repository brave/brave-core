// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Mock Data
import {
  mockMeldFiatCurrency //
} from '../../../../../common/constants/mocks'

// Components
import { AmountButton } from './amount_button'
import {
  WalletPageStory //
} from '../../../../../stories/wrappers/wallet-page-story-wrapper'

export const _AmountButton = () => {
  // State
  const [amount, setAmount] = React.useState<string>('')

  return (
    <WalletPageStory>
      <AmountButton
        labelText='Amount'
        currencyCode={mockMeldFiatCurrency.currencyCode}
        amount={amount}
        estimatedCryptoAmount='0.03535 ETH'
        onChange={setAmount}
        onClick={() => console.log('Open account selection modal')}
      />
    </WalletPageStory>
  )
}

_AmountButton.story = {
  name: 'Fund Wallet - Amount Button'
}

export default {
  component: _AmountButton
}
