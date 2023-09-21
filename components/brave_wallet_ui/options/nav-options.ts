// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// Types
import {
  NavOption,
  WalletRoutes,
  AccountPageTabs
} from '../constants/types'

export const BuySendSwapDepositOptions: NavOption[] = [
  {
    id: 'buy',
    name: 'braveWalletBuy',
    icon: 'coins-alt1',
    route: WalletRoutes.FundWalletPageStart
  },
  {
    id: 'send',
    name: 'braveWalletSend',
    icon: 'send',
    route: WalletRoutes.SendPageStart
  },
  {
    id: 'swap',
    name: 'braveWalletSwap',
    icon: 'currency-exchange',
    route: WalletRoutes.Swap
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

// We can remove this once we go live with Panel 2.0
export const PanelNavOptionsOld: NavOption[] = [
  ...BuySendSwapDepositOptions,
  ActivityNavOption
]

export const PanelNavOptions: NavOption[] = [
  {
    id: 'portfolio',
    name: 'braveWalletTopNavPortfolio',
    icon: 'coins',
    route: WalletRoutes.Portfolio
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
    route: WalletRoutes.ExploreMarket
  }
]

export const NavOptions: NavOption[] = [
  {
    id: 'portfolio',
    name: 'braveWalletTopNavPortfolio',
    icon: 'coins',
    route: WalletRoutes.Portfolio
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
    route: WalletRoutes.ExploreMarket
  }
]

export const AllNavOptions: NavOption[] = [
  ...NavOptions,
  ...BuySendSwapDepositOptions,
  ActivityNavOption
]

export const PortfolioNavOptions: NavOption[] = [
  {
    id: 'assets',
    name: 'braveWalletAccountsAssets',
    icon: 'coins',
    route: WalletRoutes.PortfolioAssets
  },
  {
    id: 'nfts',
    name: 'braveWalletTopNavNFTS',
    icon: 'grid04',
    route: WalletRoutes.PortfolioNFTs
  },
]

export const PortfolioAssetOptions: NavOption[] = [
  {
    id: 'accounts',
    name: 'braveWalletTopNavAccounts',
    icon: 'user-accounts',
    route: WalletRoutes.AccountsHash
  },
  {
    id: 'transactions',
    name: 'braveWalletTransactions',
    icon: 'activity',
    route: WalletRoutes.TransactionsHash
  }
]

export const EditVisibleAssetsOptions: NavOption[] = [
  {
    id: 'my_assets',
    name: 'braveWalletMyAssets',
    icon: '',
    route: WalletRoutes.MyAssetsHash
  },
  {
    id: 'available_assets',
    name: 'braveWalletAvailableAssets',
    icon: '',
    route: WalletRoutes.AvailableAssetsHash
  }
]

export const CreateAccountOptions: NavOption[] = [
  {
    id: 'accounts',
    name: 'braveWalletCreateAccountButton',
    icon: 'plus-add',
    route: WalletRoutes.CreateAccountModalStart
  },
  {
    id: 'accounts',
    name: 'braveWalletImportAccount',
    icon: 'product-brave-wallet',
    route: WalletRoutes.ImportAccountModalStart
  },
  {
    id: 'accounts',
    name: 'braveWalletConnectHardwareWallet',
    icon: 'flashdrive',
    route: WalletRoutes.AddHardwareAccountModalStart
  },
]

export const AccountDetailsOptions: NavOption[] = [
  {
    id: 'assets',
    name: 'braveWalletAccountsAssets',
    icon: '',
    route: AccountPageTabs.AccountAssetsSub
  },
  {
    id: 'nfts',
    name: 'braveWalletTopNavNFTS',
    icon: '',
    route: AccountPageTabs.AccountNFTsSub
  },
  {
    id: 'transactions',
    name: 'braveWalletTransactions',
    icon: '',
    route: AccountPageTabs.AccountTransactionsSub
  },
]

export const ExploreNavOptions: NavOption[] = [
  {
    id: 'market',
    name: 'braveWalletTopNavMarket',
    route: WalletRoutes.ExploreMarket,
    icon: ''
  },
  {
    id: 'web3',
    name: 'Web3',
    route: WalletRoutes.ExploreMarket,
    icon: ''
  }
]
