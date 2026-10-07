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
  PanelTypes,
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
  'transactionStatus',
]

export const isValidPanelNavigationOption = (
  panelName: string,
): panelName is PanelTypes => {
  return (
    PANEL_TYPES.includes(panelName as PanelTypes)
    && !isPersistanceOfPanelProhibited(panelName as PanelTypes)
  )
}

const BridgeOption: NavOption = {
  id: 'bridge',
  name: S.BRAVE_WALLET_BRIDGE,
  icon: 'web3-bridge',
  route: WalletRoutes.Bridge,
}

export const BuySendSwapDepositOptions: NavOption[] = [
  {
    id: 'buy',
    name: S.BRAVE_WALLET_BUY,
    icon: 'coins-alt1',
    route: WalletRoutes.BuyPageStart,
  },
  {
    id: 'send',
    name: S.BRAVE_WALLET_SEND,
    icon: 'send',
    route: WalletRoutes.Send,
  },
  {
    id: 'swap',
    name: S.BRAVE_WALLET_SWAP,
    icon: 'currency-exchange',
    route: WalletRoutes.Swap,
  },
  BridgeOption,
  {
    id: 'deposit',
    name: S.BRAVE_WALLET_DEPOSIT_CRYPTO_BUTTON,
    icon: 'money-bag-coins',
    route: WalletRoutes.DepositPageStart,
  },
]

export const BuySendSwapDepositIOSOptions: NavOption[] = [
  {
    id: 'buy',
    name: S.BRAVE_WALLET_BUY,
    icon: 'coins-alt1',
    route: WalletRoutes.BuyPageStart,
  },
  {
    id: 'send',
    name: S.BRAVE_WALLET_SEND,
    icon: 'send',
    route: WalletRoutes.Send,
  },
  {
    id: 'swap',
    name: S.BRAVE_WALLET_SWAP,
    icon: 'currency-exchange',
    route: WalletRoutes.Swap,
  },
  {
    id: 'deposit',
    name: S.BRAVE_WALLET_DEPOSIT_CRYPTO_BUTTON,
    icon: 'money-bag-coins',
    route: WalletRoutes.DepositPageStart,
  },
]

const PortfolioActivityNavOption: NavOption = {
  id: 'activity',
  name: S.BRAVE_WALLET_ACTIVITY,
  icon: 'activity',
  route: WalletRoutes.PortfolioActivity,
}

const AssetsNavOption: NavOption = {
  id: 'assets',
  name: S.BRAVE_WALLET_ACCOUNTS_ASSETS,
  icon: 'coins',
  route: WalletRoutes.PortfolioAssets,
}

const ExploreNavOption: NavOption = {
  id: 'explore',
  name: S.BRAVE_WALLET_TOP_NAV_EXPLORE,
  icon: 'discover',
  route: WalletRoutes.Explore,
}

export const PanelNavOptions: NavOption[] = [
  {
    id: 'portfolio',
    name: S.BRAVE_WALLET_TOP_NAV_PORTFOLIO,
    icon: 'coins',
    route: WalletRoutes.Portfolio,
  },
  {
    id: 'connections',
    name: S.BRAVE_WALLET_CONNECTIONS,
    icon: 'link-normal',
    route: WalletRoutes.Connections,
  },
  {
    id: 'accounts',
    name: S.BRAVE_WALLET_TOP_NAV_ACCOUNTS,
    icon: 'user-accounts',
    route: WalletRoutes.Accounts,
  },
  ExploreNavOption,
]

export const NavOptions: NavOption[] = [
  {
    id: 'portfolio',
    name: S.BRAVE_WALLET_TOP_NAV_PORTFOLIO,
    icon: 'coins',
    route: WalletRoutes.Portfolio,
  },
  {
    id: 'accounts',
    name: S.BRAVE_WALLET_TOP_NAV_ACCOUNTS,
    icon: 'user-accounts',
    route: WalletRoutes.Accounts,
  },
  ExploreNavOption,
]

export const AllNavOptions: NavOption[] = [
  ...NavOptions,
  ...BuySendSwapDepositOptions,
]

export const PortfolioNavOptions: NavOption[] = [
  AssetsNavOption,
  {
    id: 'nfts',
    name: S.BRAVE_WALLET_TOP_NAV_N_F_T_S,
    icon: 'grid04',
    route: WalletRoutes.PortfolioNFTs,
  },
  PortfolioActivityNavOption,
]

export const PortfolioNavOptionsNoNFTsTab: NavOption[] = [
  AssetsNavOption,
  PortfolioActivityNavOption,
]

export const ExploreNavOptions: NavOption[] = [
  {
    id: 'market',
    name: S.BRAVE_WALLET_TOP_NAV_MARKET,
    icon: 'discover',
    route: WalletRoutes.Market,
  },
]

export const PortfolioAssetOptions: NavOption[] = [
  {
    id: 'accounts',
    name: S.BRAVE_WALLET_TOP_NAV_ACCOUNTS,
    icon: 'user-accounts',
    route: WalletRoutes.AccountsHash,
  },
  {
    id: 'transactions',
    name: S.BRAVE_WALLET_TRANSACTIONS,
    icon: 'activity',
    route: WalletRoutes.TransactionsHash,
  },
]

export const EditVisibleAssetsOptions: NavOption[] = [
  {
    id: 'my_assets',
    name: S.BRAVE_WALLET_MY_ASSETS,
    icon: '',
    route: WalletRoutes.MyAssetsHash,
  },
  {
    id: 'available_assets',
    name: S.BRAVE_WALLET_AVAILABLE_ASSETS,
    icon: '',
    route: WalletRoutes.AvailableAssetsHash,
  },
]

export const CreateAccountOptions: NavOption[] = [
  {
    id: 'accounts',
    name: S.BRAVE_WALLET_CREATE_ACCOUNT_BUTTON,
    icon: 'plus-add',
    route: WalletRoutes.CreateAccountModalStart,
  },
  {
    id: 'accounts',
    name: S.BRAVE_WALLET_IMPORT_ACCOUNT,
    icon: 'import-arrow',
    route: WalletRoutes.ImportAccountModalStart,
  },
  {
    id: 'accounts',
    name: S.BRAVE_WALLET_CONNECT_HARDWARE_WALLET,
    icon: 'flashdrive',
    route: WalletRoutes.AddHardwareAccountModalStart,
  },
  {
    id: 'accounts',
    name: S.BRAVE_WALLET_WELCOME_RESTORE_BUTTON,
    icon: 'reload',
    route: WalletRoutes.RestoreAccountsModal,
  },
]

export const AccountDetailsOptions: NavOption[] = [
  {
    id: 'assets',
    name: S.BRAVE_WALLET_ACCOUNTS_ASSETS,
    icon: '',
    route: AccountPageTabs.AccountAssetsSub,
  },
  {
    id: 'nfts',
    name: S.BRAVE_WALLET_TOP_NAV_N_F_T_S,
    icon: '',
    route: AccountPageTabs.AccountNFTsSub,
  },
  {
    id: 'transactions',
    name: S.BRAVE_WALLET_TRANSACTIONS,
    icon: '',
    route: AccountPageTabs.AccountTransactionsSub,
  },
]
