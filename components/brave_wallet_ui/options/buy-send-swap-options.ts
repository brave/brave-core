// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { BuySendSwapObjectType } from '../constants/types'
import { getLocale } from '../../common/locale'

export const BuySendSwapOptions = (): BuySendSwapObjectType[] => [
  {
    id: 'buy',
    name: getLocale('braveWalletBuy')
  },
  {
    id: 'send',
    name: getLocale('braveWalletSend')
  },
  {
    id: 'swap',
    name: getLocale('braveWalletSwap')
  }
]
