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
import StripeIcon from '../assets/svg-icons/stripe-logo.svg'
import CoinbaseIcon from '../assets/svg-icons/coinbase-logo.svg'
import { isStripeSupported } from '../utils/asset-utils'

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
  },
  {
    id: BraveWallet.OnRampProvider.kSardine,
    actionText: getLocale('braveWalletBuyWithSardine'),
    icon: window.matchMedia('(prefers-color-scheme: dark)').matches
      ? SardineIconDark
      : SardineIconLight,
    name: getLocale('braveWalletBuySardineName'),
    description: getLocale('braveWalletBuySardineDescription')
  },
  {
    id: BraveWallet.OnRampProvider.kCoinbase,
    actionText: getLocale('braveWalletBuyWithCoinbase'),
    icon: CoinbaseIcon,
    name: getLocale('braveWalletBuyCoinbaseName'),
    description: getLocale('braveWalletBuyCoinbaseDescription')
  }
]

  if(isStripeSupported()) {
    buyOptions.push({
      id: BraveWallet.OnRampProvider.kStripe,
      actionText: getLocale('braveWalletBuyWithStripe'),
      icon: StripeIcon,
      name: getLocale('braveWalletBuyStripeName'),
      description: getLocale('braveWalletBuyStripeDescription')
    })
  }

  return buyOptions
}

export const BuyOptions = getBuyOptions()
