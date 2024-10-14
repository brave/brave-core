// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  AccountPageTabs,
  BraveWallet,
  WalletRoutes,
  SendPageTabHashes,
  WalletOrigin,
  WalletCreationMode,
  WalletImportMode,
  NftDropdownOptionId
} from '../constants/types'
import { LOCAL_STORAGE_KEYS } from '../common/constants/local-storage-keys'
import { SUPPORT_LINKS } from '../common/constants/support_links'

/**
 * Checks the provided route against a list of routes that we are OK with the
 * wallet opening to when the app is unlocked or when the panel is re-opened
 */
export function isPersistableSessionRoute(
  route?: string,
  isPanel?: boolean
): route is WalletRoutes {
  if (!route) {
    return false
  }
  const isPersistableInPanel =
    route.includes(WalletRoutes.Accounts) ||
    route.includes(WalletRoutes.Backup) ||
    route.includes(WalletRoutes.DepositFundsPageStart) ||
    route.includes(WalletRoutes.FundWalletPageStart) ||
    route.includes(WalletRoutes.PortfolioAssets) ||
    route.includes(WalletRoutes.PortfolioNFTs) ||
    route.includes(WalletRoutes.PortfolioNFTAsset) ||
    route.includes(WalletRoutes.PortfolioActivity) ||
    route.includes(WalletRoutes.Market) ||
    route.includes(WalletRoutes.Explore)
  if (isPanel) {
    return isPersistableInPanel
  }
  return (
    isPersistableInPanel ||
    route.includes(WalletRoutes.Swap) ||
    route.includes(WalletRoutes.Send) ||
    route.includes(WalletRoutes.Bridge)
  )
}

export function getInitialSessionRoute(
  isPanel?: boolean
): WalletRoutes | undefined {
  const route =
    window.localStorage.getItem(LOCAL_STORAGE_KEYS.SAVED_SESSION_ROUTE) || ''
  return isPersistableSessionRoute(route, isPanel) ? route : undefined
}

export function getOnboardingTypeFromPath(
  path: WalletRoutes | string
): WalletCreationMode {
  if (path.includes(WalletRoutes.OnboardingHardwareWalletStart)) {
    return 'hardware'
  }
  if (path.includes(WalletRoutes.OnboardingImportStart)) {
    return 'import'
  }
  return 'new'
}

export function getOnboardingImportTypeFromPath(
  path: WalletRoutes | string
): WalletImportMode {
  if (path.includes(WalletRoutes.OnboardingRestoreWallet)) {
    return 'seed'
  }
  if (path.includes(WalletRoutes.OnboardingImportMetaMask)) {
    return 'metamask'
  }
  return 'legacy'
}

export const makeAccountRoute = (
  accountInfo: Pick<BraveWallet.AccountInfo, 'address' | 'accountId'>,
  tab: AccountPageTabs
) => {
  const id = accountInfo.address || accountInfo.accountId.uniqueKey
  return WalletRoutes.Account.replace(':accountId', id).replace(
    ':selectedTab?',
    tab
  )
}

export const makeAccountTransactionRoute = (
  accountInfo: Pick<BraveWallet.AccountInfo, 'address' | 'accountId'>,
  transactionId: string
) => {
  const id = accountInfo.address || accountInfo.accountId.uniqueKey
  return (
    WalletRoutes.Account.replace(':accountId', id).replace(
      ':selectedTab?',
      AccountPageTabs.AccountTransactionsSub
    ) +
    '#' +
    transactionId.replace('#', '')
  )
}

export const makeFundWalletRoute = (
  assetId: string,
  options?: {
    currencyCode?: string
    buyAmount?: string
    searchText?: string
    chainId?: string
    coinType?: string
  }
) => {
  if (options) {
    const params = new URLSearchParams()

    if (options.currencyCode) {
      params.append('currencyCode', options.currencyCode)
    }
    if (options.buyAmount) {
      params.append('buyAmount', options.buyAmount)
    }
    if (options.searchText) {
      params.append('search', options.searchText)
    }
    if (options.chainId) {
      params.append('chainId', options.chainId)
    }
    if (options.coinType) {
      params.append('coinType', options.coinType)
    }

    return `${WalletRoutes.FundWalletPage.replace(
      ':assetId?',
      assetId
    )}?${params.toString()}`
  }
  return WalletRoutes.FundWalletPage.replace(':assetId?', assetId)
}

export const makeFundWalletPurchaseOptionsRoute = (
  assetId: string,
  options?: {
    currencyCode: string
    buyAmount: string
  }
) => {
  if (options) {
    const params = new URLSearchParams()
    if (options.currencyCode) {
      params.append('currencyCode', options.currencyCode)
    }
    if (options.buyAmount) {
      params.append('buyAmount', options.buyAmount)
    }

    return `${WalletRoutes.FundWalletPurchaseOptionsPage.replace(
      ':assetId',
      assetId
    )}?${params.toString()}`
  }

  return WalletRoutes.FundWalletPurchaseOptionsPage.replace(
    ':assetId', //
    assetId
  )
}

export const makeDepositFundsRoute = (
  assetId: string,
  options?: {
    searchText?: string
    chainId?: string
    coinType?: string
  }
) => {
  if (options) {
    const params = new URLSearchParams()
    if (options.searchText) {
      params.append('search', options.searchText)
    }
    if (options.chainId) {
      params.append('chainId', options.chainId)
    }
    if (options.coinType) {
      params.append('coinType', options.coinType)
    }

    return `${WalletRoutes.DepositFundsPage.replace(
      ':assetId?',
      assetId
    )}?${params.toString()}`
  }

  return WalletRoutes.DepositFundsPage.replace(':assetId?', assetId)
}

export const makeDepositFundsAccountRoute = (assetId: string) => {
  return WalletRoutes.DepositFundsAccountPage.replace(':assetId', assetId)
}

export const makeSendRoute = (
  asset: BraveWallet.BlockchainToken,
  account: BraveWallet.AccountInfo
) => {
  const isNftTab = asset.isErc721 || asset.isNft
  const baseQueryParams = {
    chainId: asset.chainId,
    token: asset.contractAddress || asset.symbol.toUpperCase(),
    account: account.accountId.uniqueKey
  }
  const params = new URLSearchParams(
    asset.tokenId
      ? { ...baseQueryParams, tokenId: asset.tokenId }
      : baseQueryParams
  )

  if (isNftTab) {
    return `${WalletRoutes.Send}?${params.toString()}${SendPageTabHashes.nft}`
  }

  return `${WalletRoutes.Send}?${params.toString()}${SendPageTabHashes.token}`
}

export const makeSwapOrBridgeRoute = ({
  fromToken,
  fromAccount,
  toToken,
  toAddress,
  toCoin,
  routeType
}: {
  fromToken: BraveWallet.BlockchainToken
  fromAccount: BraveWallet.AccountInfo
  toToken?: BraveWallet.BlockchainToken
  toAddress?: string
  toCoin?: BraveWallet.CoinType
  routeType?: 'swap' | 'bridge'
}) => {
  const baseQueryParams = {
    fromChainId: fromToken.chainId,
    fromToken: fromToken.contractAddress || fromToken.symbol.toUpperCase(),
    fromAccountId: fromAccount.accountId.uniqueKey,
    toChainId: toToken ? toToken.chainId : fromToken.chainId,
    toAddress: toAddress ?? fromAccount.accountId.address,
    toCoin: toCoin ? toCoin.toString() : fromAccount.accountId.coin.toString()
  }

  const toTokenParam = toToken
    ? toToken.contractAddress || toToken.symbol.toUpperCase()
    : undefined

  const params = new URLSearchParams(
    toTokenParam
      ? { ...baseQueryParams, toToken: toTokenParam }
      : baseQueryParams
  )

  const route = routeType === 'bridge' ? WalletRoutes.Bridge : WalletRoutes.Swap

  return `${route}?${params.toString()}`
}

export const makeTransactionDetailsRoute = (transactionId: string) => {
  return WalletRoutes.PortfolioActivity + `#${transactionId}`
}

export const makePortfolioAssetRoute = (isNft: boolean, assetId: string) => {
  return (
    isNft ? WalletRoutes.PortfolioNFTAsset : WalletRoutes.PortfolioAsset
  ).replace(':assetId', assetId)
}

export const makePortfolioNftCollectionRoute = (
  collectionName: string,
  page?: number
) => {
  if (page) {
    const params = new URLSearchParams({
      page: page.toString()
    })
    return `${WalletRoutes.PortfolioNFTCollection.replace(
      ':collectionName',
      collectionName
    )}?${params.toString()}`
  }
  return WalletRoutes.PortfolioNFTCollection.replace(
    ':collectionName',
    collectionName
  )
}

export const makePortfolioNftsRoute = (
  tab: NftDropdownOptionId,
  page?: number
) => {
  const params = new URLSearchParams({
    tab: tab,
    page: page?.toString() || '1'
  })
  return `${WalletRoutes.PortfolioNFTs}?${params.toString()}`
}

// Tabs
export function openTab(url: string) {
  if (chrome.tabs !== undefined) {
    chrome.tabs.create({ url }, () => {
      if (chrome.runtime.lastError) {
        console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
      }
    })
  } else {
    // Tabs.create is desktop specific. Using window.open for android.
    window.open(url, '_blank', 'noopener noreferrer')
  }
}

// Wallet Page Tabs
export const openWalletRouteTab = (route: WalletRoutes) => {
  openTab(`${WalletOrigin}${route}`)
}

// Settings tabs
export function openWalletSettings() {
  openTab('chrome://settings/web3')
}

export function openNetworkSettings() {
  openTab('chrome://settings/wallet/networks')
}

export const openSupportTab = (key: keyof typeof SUPPORT_LINKS) => {
  const url = SUPPORT_LINKS[key]
  if (!url) {
    console.error(`support link not found (${key})`)
  }
  chrome.tabs.create({ url }, () => {
    if (chrome.runtime.lastError) {
      console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
    }
  })
}

export const openAssociatedTokenAccountSupportArticleTab = () => {
  openSupportTab('WhatIsTheAssociatedTokenAccountProgram')
}
