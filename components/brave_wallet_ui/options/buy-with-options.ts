/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { getLocale } from '$web-common/locale'
import { BraveWallet, BuyOption } from '../constants/types'

import RampIcon from '../assets/svg-icons/ramp-icon.svg'
import SardineIconLight from '../assets/svg-icons/sardine-logo-light.svg'
import SardineIconDark from '../assets/svg-icons/sardine-logo-dark.svg'
import TransakIcon from '../assets/svg-icons/transak-logo.svg'

import { isSardineSupported } from '../utils/asset-utils'

function getBuyOptions (): BuyOption[] {
  const buyOptions = [{
    id: BraveWallet.OnRampProvider.kRamp,
    actionText: getLocale('braveWalletBuyWithRamp'),
    icon: RampIcon,
    name: getLocale('braveWalletBuyRampNetworkName'),
    description: getLocale('braveWalletBuyRampDescription')
  },
  {
    id: BraveWallet.OnRampProvider.kTransak,
    actionText: getLocale('braveWalletBuyWithTransak'),
    icon: TransakIcon,
    name: getLocale('braveWalletBuyTransakName'),
    description: getLocale('braveWalletBuyTransakDescription')
  }
  ]

  if (isSardineSupported()) {
    buyOptions.push({
      id: BraveWallet.OnRampProvider.kSardine,
      actionText: getLocale('braveWalletBuyWithSardine'),
      icon: window.matchMedia('(prefers-color-scheme: dark)').matches
        ? SardineIconDark
        : SardineIconLight,
      name: getLocale('braveWalletBuySardineName'),
      description: getLocale('braveWalletBuySardineDescription')
    })
  }

  return buyOptions
}

export const BuyOptions = getBuyOptions()
