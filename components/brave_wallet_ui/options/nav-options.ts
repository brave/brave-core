// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// Utils
import { isPersistanceOfPanelProhibited } from '../utils/local-storage-utils'

// Types
import {
  NavOption,
  WalletRoutes,
  AccountPageTabs,
  PanelTypes
} from '../constants/types'

const PANEL_TYPES: PanelTypes[] = [
  'accounts',
  'approveTransaction',
  'assets',
  'buy',
  'connectHardwareWallet',
  'connectWithSite',
  'createAccount',
  'expanded',
  'main',
  'networks',
  'send',
  'settings',
  'sitePermissions',
  'swap',
  'activity', // Transactions
  'transactionStatus'
]

export const isValidPanelNavigationOption = (
  panelName: string
): panelName is PanelTypes => {
  return (
    PANEL_TYPES.includes(panelName as PanelTypes) &&
    !isPersistanceOfPanelProhibited(panelName as PanelTypes)
  )
}

const BridgeOption: NavOption = {
  id: 'bridge',
  name: 'braveWalletBridge',
  icon: 'web3-bridge',
  route: WalletRoutes.Bridge
}

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
    route: WalletRoutes.Send
  },
  {
    id: 'swap',
    name: 'braveWalletSwap',
    icon: 'currency-exchange',
    route: WalletRoutes.Swap
  },
  BridgeOption,
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

const PortfolioActivityNavOption: NavOption = {
  id: 'activity',
  name: 'braveWalletActivity',
  icon: 'activity',
  route: WalletRoutes.PortfolioActivity
}

const AssetsNavOption: NavOption = {
  id: 'assets',
  name: 'braveWalletAccountsAssets',
  icon: 'coins',
  route: WalletRoutes.PortfolioAssets
}

const ExploreNavOption: NavOption = {
  id: 'explore',
  name: 'braveWalletTopNavExplore',
  icon: 'discover',
  route: WalletRoutes.Explore
}

export const PanelNavOptions: NavOption[] = [
  {
    id: 'portfolio',
    name: 'braveWalletTopNavPortfolio',
    icon: 'coins',
    route: WalletRoutes.Portfolio
  },
  {
    id: 'accounts',
    name: 'braveWalletTopNavAccounts',
    icon: 'user-accounts',
    route: WalletRoutes.Accounts
  },
  ExploreNavOption
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
  ExploreNavOption
]

export const AllNavOptions: NavOption[] = [
  ...NavOptions,
  ...BuySendSwapDepositOptions,
  ActivityNavOption
]

export const PortfolioNavOptions: NavOption[] = [
  AssetsNavOption,
  {
    id: 'nfts',
    name: 'braveWalletTopNavNFTS',
    icon: 'grid04',
    route: WalletRoutes.PortfolioNFTs
  }
]

export const PortfolioPanelNavOptions: NavOption[] = [
  ...PortfolioNavOptions,
  PortfolioActivityNavOption
]

export const PortfolioPanelNavOptionsNoNFTsTab: NavOption[] = [
  AssetsNavOption,
  PortfolioActivityNavOption
]

export const ExploreNavOptions: NavOption[] = [
  {
    id: 'market',
    name: 'braveWalletTopNavMarket',
    icon: 'discover',
    route: WalletRoutes.Market
  },
  {
    id: 'web3',
    name: 'braveWalletWeb3',
    icon: 'discover',
    route: WalletRoutes.Web3
  }
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
  }
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
  }
]
