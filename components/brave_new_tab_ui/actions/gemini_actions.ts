// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { action } from 'typesafe-actions'
import { types } from '../constants/gemini_types'

export const onGeminiClientUrl = (clientUrl: string) => action(types.ON_GEMINI_CLIENT_URL, {
  clientUrl
})

export const onValidGeminiAuthCode = () => action(types.ON_VALID_GEMINI_AUTH_CODE)

export const connectToGemini = () => action(types.CONNECT_TO_GEMINI)

export const setGeminiTickerPrice = (asset: string, price: string) => action(types.SET_GEMINI_TICKER_PRICE, {
  asset,
  price
})
