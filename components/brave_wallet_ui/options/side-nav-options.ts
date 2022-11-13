// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { NavObjectType } from '../constants/types'
import { getLocale } from '../../common/locale'
import {
  WalletIconD,
  WalletIconL,
  RewardsIconL,
  RewardsIconD,
  CreditCardIconL,
  CreditCardIconD
} from '../assets/svg-icons/nav-button-icons'

export const NavOptions = (): NavObjectType[] => [
  {
    name: getLocale('braveWalletSideNavCrypto'),
    primaryIcon: WalletIconL,
    secondaryIcon: WalletIconD,
    id: 'crypto'
  },
  {
    name: getLocale('braveWalletSideNavRewards'),
    primaryIcon: RewardsIconL,
    secondaryIcon: RewardsIconD,
    id: 'rewards'
  },
  {
    name: getLocale('braveWalletSideNavCards'),
    primaryIcon: CreditCardIconL,
    secondaryIcon: CreditCardIconD,
    id: 'cards'
  }
]
