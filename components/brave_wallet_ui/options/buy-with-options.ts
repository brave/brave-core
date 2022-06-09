/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { getLocale } from '$web-common/locale'
import { BraveWallet, BuyOption } from '../constants/types'

import RampIcon from '../assets/svg-icons/ramp-icon.svg'
import WyreIcon from '../assets/svg-icons/wyre-icon.svg'

export const BuyOptions: BuyOption[] = [
  {
    id: BraveWallet.OnRampProvider.kRamp,
    actionText: getLocale('braveWalletBuyWithRamp'),
    icon: RampIcon,
    name: getLocale('braveWalletBuyRampNetworkName'),
    description: getLocale('braveWalletBuyRampDescription')
  },
  {
    id: BraveWallet.OnRampProvider.kWyre,
    actionText: getLocale('braveWalletBuyWithWyre'),
    icon: WyreIcon,
    name: getLocale('braveWalletBuyWyreName'),
    description: getLocale('braveWalletBuyWyreDescription')
  }
]
