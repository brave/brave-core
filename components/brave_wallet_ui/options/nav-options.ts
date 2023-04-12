// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// Types
import { NavOption, WalletRoutes } from '../constants/types'

export const BuySendSwapDepositOptions: NavOption[] = [
  {
    id: 'buy',
    name: 'braveWalletBuy',
    icon: 'coins-alt1',
    route: WalletRoutes.FundWalletPageStart
  },
  {
    id: 'swap',
    name: 'braveWalletSwap',
    icon: 'currency-exchange',
    route: WalletRoutes.Swap
  },
  {
    id: 'send',
    name: 'braveWalletSend',
    icon: 'send',
    route: WalletRoutes.Send
  },
  {
    id: 'deposit',
    name: 'braveWalletDepositCryptoButton',
    icon: 'money-bag-coins',
    route: WalletRoutes.DepositFundsPageStart
  }
]

const ActivityNavOption: NavOption = {
  id: 'activity',
  name: 'braveWalletActivity',
  icon: 'activity',
  route: WalletRoutes.Activity
}

export const PanelNavOptions: NavOption[] = [
  ...BuySendSwapDepositOptions,
  ActivityNavOption
]

export const NavOptions: NavOption[] = [
  {
    id: 'portfolio',
    name: 'braveWalletTopNavPortfolio',
    icon: 'coins',
    route: WalletRoutes.Portfolio
  },
  {
    id: 'nfts',
    name: 'braveWalletTopNavNFTS',
    icon: 'grid04',
    route: WalletRoutes.Nfts
  },
  ActivityNavOption,
  {
    id: 'accounts',
    name: 'braveWalletTopNavAccounts',
    icon: 'user-accounts',
    route: WalletRoutes.Accounts
  },
  {
    id: 'market',
    name: 'braveWalletTopNavMarket',
    icon: 'discover',
    route: WalletRoutes.Market
  }
]

export const AllNavOptions: NavOption[] = [
  ...NavOptions,
  ...BuySendSwapDepositOptions,
  ActivityNavOption
]
