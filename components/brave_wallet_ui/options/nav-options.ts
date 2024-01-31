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
  'addEthereumChain',
  'allowReadingEncryptedMessage', // For grep: 'decryptRequest'
  'approveTransaction',
  'assets',
  'buy',
  'connectHardwareWallet',
  'connectWithSite',
  'createAccount',
  'expanded',
  'main',
  'networks',
  'provideEncryptionKey', // For grep: 'getEncryptionPublicKey'
  'send',
  'settings',
  'showUnlock',
  'signData',
  'signTransaction',
  'signAllTransactions',
  'sitePermissions',
  'swap',
  'switchEthereumChain',
  'transactionDetails',
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
  ActivityNavOption,
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
  }
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

export const SendSwapBridgeOptions: NavOption[] = [
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
  }
  // Bridge is not yet implemented
  // {
  //   id: 'bridge',
  //   name: 'braveWalletBridge',
  //   icon: 'bridge',
  //   route: WalletRoutes.Bridge
  // }
]
