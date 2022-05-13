/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { getLocale } from '$web-common/locale'
import { BraveWallet, BuyOption } from '../constants/types'

export const BuyOptions: BuyOption[] = [
  {
    id: BraveWallet.OnRampProvider.kRamp,
    label: getLocale('braveWalletBuyWithRamp')
  },
  {
    id: BraveWallet.OnRampProvider.kWyre,
    label: getLocale('braveWalletBuyWithWyre')
  }
]
