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
  WalletImportMode
} from '../constants/types'
import { LOCAL_STORAGE_KEYS } from '../common/constants/local-storage-keys'

/**
 * Checks the provided route against a list of routes that we are OK with the
 * wallet opening to when the app is unlocked or when the panel is re-opened
 */
export function isPersistableSessionRoute(
  route?: string
): route is WalletRoutes {
  if (!route) {
    return false
  }
  return (
    route.includes(WalletRoutes.Accounts) ||
    route.includes(WalletRoutes.Activity) ||
    route.includes(WalletRoutes.Backup) ||
    route.includes(WalletRoutes.DepositFundsPageStart) ||
    route.includes(WalletRoutes.FundWalletPageStart) ||
    route.includes(WalletRoutes.PortfolioAssets) ||
    route.includes(WalletRoutes.PortfolioNFTs) ||
    route.includes(WalletRoutes.PortfolioNFTAsset) ||
    route.includes(WalletRoutes.Market) ||
    route.includes(WalletRoutes.Swap) ||
    route.includes(WalletRoutes.Send) ||
    route.includes(WalletRoutes.LocalIpfsNode) ||
    route.includes(WalletRoutes.InspectNfts)
  )
}

export function getInitialSessionRoute(): WalletRoutes | undefined {
  const route =
    window.localStorage.getItem(LOCAL_STORAGE_KEYS.SESSION_ROUTE) || ''
  return isPersistableSessionRoute(route) ? route : undefined
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

export const makeFundWalletRoute = ({
  buyAmount,
  chainId,
  coinType,
  currencyCode,
  searchText,
  assetId
}: {
  assetId?: string
  currencyCode?: string
  buyAmount?: string
  searchText?: string
  chainId?: string
  coinType?: string
}) => {
  const params = new URLSearchParams()
  if (assetId) {
    params.append('assetId', assetId)
  }
  if (currencyCode) {
    params.append('currencyCode', currencyCode)
  }
  if (buyAmount) {
    params.append('buyAmount', buyAmount)
  }
  if (searchText) {
    params.append('search', searchText)
  }
  if (chainId) {
    params.append('chainId', chainId)
  }
  if (coinType) {
    params.append('coinType', coinType)
  }

  const paramsString = params ? params.toString() : undefined

  return `${WalletRoutes.FundWalletPage}${
    paramsString ? `?${paramsString}` : ''
  }`
}

export const makeFundWalletPurchaseOptionsRoute = ({
  buyAmount,
  currencyCode,
  assetId
}: {
  currencyCode: string
  buyAmount: string
  assetId?: string
}) => {
  const params = new URLSearchParams()
  if (assetId) {
    params.append('assetId', assetId)
  }
  if (currencyCode) {
    params.append('currencyCode', currencyCode)
  }
  if (buyAmount) {
    params.append('buyAmount', buyAmount)
  }

  const paramsString = params ? params.toString() : undefined

  return `${WalletRoutes.FundWalletPurchaseOptionsPage}${
    paramsString ? `?${paramsString}` : ''
  }`
}

export const makeDepositFundsRoute = ({
  assetId,
  chainId,
  coinType,
  searchText
}: {
  searchText?: string
  chainId?: string
  coinType?: string
  assetId?: string
}) => {
  const params = new URLSearchParams()
  if (assetId) {
    params.append('assetId', assetId)
  }
  if (searchText) {
    params.append('search', searchText)
  }
  if (chainId) {
    params.append('chainId', chainId)
  }
  if (coinType) {
    params.append('coinType', coinType)
  }

  const paramsString = params ? params.toString() : undefined

  return `${WalletRoutes.DepositFundsPage}${
    paramsString ? `?${paramsString}` : ''
  }`
}

export const makeDepositFundsAccountRoute = (assetId: string) => {
  const params = new URLSearchParams()
  if (assetId) {
    params.append('assetId', assetId)
  }

  return `${WalletRoutes.DepositFundsAccountPage}?${params.toString()}`
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

export const makePortfolioAssetRoute = (isNft: boolean, assetId: string) => {
  return (
    isNft ? WalletRoutes.PortfolioNFTAsset : WalletRoutes.PortfolioAsset
  ).replace(':assetId', assetId)
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
