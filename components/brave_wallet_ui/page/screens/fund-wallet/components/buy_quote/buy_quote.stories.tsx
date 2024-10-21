// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// Types
import { MeldCryptoQuote } from 'components/brave_wallet_ui/constants/types'

// Mock Data
import {
  mockMeldCryptoQuotes,
  mockServiceProviders
} from '../../../../../common/constants/mocks'

// Components
import { BuyQuote } from './buy_quote'

export const _BuyQuote = () => {
  return (
    <BuyQuote
      quote={mockMeldCryptoQuotes[0]}
      serviceProviders={mockServiceProviders}
      isCreatingWidget={false}
      onBuy={function (quote: MeldCryptoQuote): void {
        console.log(quote)
      }}
    />
  )
}

export default {
  component: _BuyQuote,
  title: 'Fund Wallet - Buy Quote'
}
