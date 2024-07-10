// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { BuyQuote } from './buy_quote'
import {
  mockMeldCryptoQuotes,
  mockServiceProviders
} from '../../../../../common/constants/mocks'

export const _BuyQuote = () => {
  return (
    <BuyQuote
      quote={mockMeldCryptoQuotes[0]}
      serviceProviders={mockServiceProviders}
    />
  )
}

export default {
  component: _BuyQuote,
  title: 'Fund Wallet - Buy Quote'
}
