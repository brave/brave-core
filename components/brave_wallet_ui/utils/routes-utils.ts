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
  NftDropdownOptionId,
  MeldCryptoCurrency,
} from '../constants/types'
import { LOCAL_STORAGE_KEYS } from '../common/constants/local-storage-keys'
import { SUPPORT_LINKS } from '../common/constants/support_links'

/**
 * Checks the provided route against a list of routes that we are OK with the
 * wallet opening to when the app is unlocked or when the panel is re-opened
 */
export function isPersistableSessionRoute(
  route?: string,
  isPanel?: boolean,
): route is WalletRoutes {
  if (!route) {
    return false
  }
  const isPersistableInPanel =
    /**
     * Insure that the Accounts route is an exact match.
     */
    route === WalletRoutes.Accounts
    /**
     * or allow if it includes a trailing slash which is followed
     * by an accountId query param.
     */
    || route.includes(WalletRoutes.Accounts + '/')
    /**
     * The Backup route uses a query param to determine the step
     * and can not be exact matched.
     */
    || route.includes(WalletRoutes.Backup)
    /**
     * Insure that the Deposit Funds route is an exact match.
     */
    || route === WalletRoutes.DepositFundsPageStart
    /**
     * or allow if it includes a trailing slash which is followed
     * by a currencyCode query param.
     */
    || route.includes(WalletRoutes.DepositFundsPageStart + '/')
    /**
     * Fund wallet route uses a query param to determine the asset
     * and can not be exact matched.
     */
    || route.includes(WalletRoutes.FundWalletPageStart)
    /**
     * Insure that the Portfolio Assets route is an exact match.
     */
    || route === WalletRoutes.PortfolioAssets
    /**
     * or allow if it includes a trailing slash which is followed
     * by a tokenId query param.
     */
    || route.includes(WalletRoutes.PortfolioAssets + '/')
    /**
     * Insure that the Portfolio NFTs route is an exact match.
     */
    || route === WalletRoutes.PortfolioNFTs
    /**
     * or allow if it includes a trailing slash which is followed
     * by a tokenId query param.
     */
    || route.includes(WalletRoutes.PortfolioNFTs + '/')
    /**
     * Portfolio Activity route uses a query param to determine the
     * transactionId and can not be exact matched.
     */
    || route.includes(WalletRoutes.PortfolioActivity)
    /**
     * Insure that the Market route is an exact match.
     */
    || route === WalletRoutes.Market
    /**
     * or allow if it includes a trailing slash which is followed
     * by a tokenId query param.
     */
    || route.includes(WalletRoutes.Market + '/')
    /**
     * Web3 route uses a query param to determine the
     * dappCategory and can not be exact matched.
     */
    || route.includes(WalletRoutes.Web3)
    /**
     * Insure that the Connections route is an exact match.
     */
    || route === WalletRoutes.Connections
    /**
     * NFT Collections route uses a query param to determine the
     * collectionName and can not be exact matched.
     */
    || route.includes(WalletRoutes.PortfolioNFTCollectionsStart)
  if (isPanel) {
    return isPersistableInPanel
  }
  return (
    isPersistableInPanel
    /**
     * The Swap route uses a query param to determine the
     * fromToken and toToken and can not be exact matched.
     */
    || route.includes(WalletRoutes.Swap)
    /**
     * The Send route uses a query param to determine the
     * asset and can not be exact matched.
     */
    || route.includes(WalletRoutes.Send)
    /**
     * The Bridge route uses a query param to determine the
     * fromToken and toToken and can not be exact matched.
     */
    || route.includes(WalletRoutes.Bridge)
  )
}

export function getInitialSessionRoute(
  isPanel?: boolean,
): WalletRoutes | undefined {
  const route =
    window.localStorage.getItem(LOCAL_STORAGE_KEYS.SAVED_SESSION_ROUTE) || ''
  return isPersistableSessionRoute(route, isPanel) ? route : undefined
}

export function getOnboardingTypeFromPath(
  path: WalletRoutes | string,
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
  path: WalletRoutes | string,
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
  tab: AccountPageTabs,
) => {
  const id = accountInfo.address || accountInfo.accountId.uniqueKey
  return WalletRoutes.Account.replace(':accountId', id).replace(
    ':selectedTab?',
    tab,
  )
}

export const makeAccountTransactionRoute = (
  accountInfo: Pick<BraveWallet.AccountInfo, 'address' | 'accountId'>,
  transactionId: string,
) => {
  const id = accountInfo.address || accountInfo.accountId.uniqueKey
  return (
    WalletRoutes.Account.replace(':accountId', id).replace(
      ':selectedTab?',
      AccountPageTabs.AccountTransactionsSub,
    )
    + '#'
    + transactionId.replace('#', '')
  )
}

export const makeFundWalletRoute = (
  asset: Pick<MeldCryptoCurrency, 'chainId' | 'currencyCode'>,
  account?: BraveWallet.AccountInfo,
) => {
  const baseQueryParams = {
    currencyCode: asset.currencyCode ?? '',
    chainId: asset.chainId ?? '',
  }

  const params = new URLSearchParams(
    account
      ? { ...baseQueryParams, accountId: account.accountId.uniqueKey }
      : baseQueryParams,
  )

  return `${WalletRoutes.FundWalletPageStart}?${params.toString()}`
}

export const makeDepositFundsRoute = (
  assetId: string,
  options?: {
    searchText?: string
    chainId?: string
    coinType?: string
  },
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
      assetId,
    )}?${params.toString()}`
  }

  return WalletRoutes.DepositFundsPage.replace(':assetId?', assetId)
}

export const makeDepositFundsAccountRoute = (assetId: string) => {
  return WalletRoutes.DepositFundsAccountPage.replace(':assetId', assetId)
}

export const makeSendRoute = (
  asset: BraveWallet.BlockchainToken,
  account?: BraveWallet.AccountInfo,
  recipient?: string,
) => {
  const isNftTab = asset.isErc721 || asset.isNft
  const baseQueryParams = {
    chainId: asset.chainId,
    token: asset.contractAddress || asset.symbol.toUpperCase(),
  }

  const accountIdQueryParams = account
    ? { ...baseQueryParams, account: account.accountId.uniqueKey }
    : baseQueryParams

  const tokenIdQueryParams = asset.tokenId
    ? { ...accountIdQueryParams, tokenId: asset.tokenId }
    : accountIdQueryParams

  const recipientQueryParams = recipient
    ? { ...tokenIdQueryParams, recipient: recipient }
    : tokenIdQueryParams

  const params = new URLSearchParams(
    asset.isShielded
      ? { ...recipientQueryParams, isShielded: 'true' }
      : recipientQueryParams,
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
  routeType,
}: {
  fromToken: BraveWallet.BlockchainToken
  fromAccount?: BraveWallet.AccountInfo
  toToken?: BraveWallet.BlockchainToken
  toAddress?: string
  toCoin?: BraveWallet.CoinType
  routeType?: 'swap' | 'bridge'
}) => {
  const baseQueryParams = {
    fromChainId: fromToken.chainId,
    fromToken: fromToken.contractAddress || fromToken.symbol.toUpperCase(),
    toChainId: toToken ? toToken.chainId : fromToken.chainId,
  }

  const fromAccountParams = fromAccount
    ? {
        ...baseQueryParams,
        fromAccountId: fromAccount.accountId.uniqueKey,
        // Will default to fromAccount's address and be replaced
        // below if toAddress is passed.
        toAddress: fromAccount.accountId.address,
        // Will default to fromAccount's coin and be replaced
        // below if toCoin is passed.
        toCoin: fromAccount.accountId.coin.toString(),
      }
    : baseQueryParams

  const toAddressParams = toAddress
    ? { ...fromAccountParams, toAddress: toAddress }
    : fromAccountParams

  const toCoinParams = toCoin
    ? { ...toAddressParams, toCoin: toCoin.toString() }
    : toAddressParams

  const toTokenParams = toToken
    ? {
        ...toCoinParams,
        toToken: toToken.contractAddress || toToken.symbol.toUpperCase(),
      }
    : toCoinParams

  const params = new URLSearchParams(toTokenParams)

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
  page?: number,
) => {
  if (page) {
    const params = new URLSearchParams({
      page: page.toString(),
    })
    return `${WalletRoutes.PortfolioNFTCollection.replace(
      ':collectionName',
      collectionName,
    )}?${params.toString()}`
  }
  return WalletRoutes.PortfolioNFTCollection.replace(
    ':collectionName',
    collectionName,
  )
}

export const makePortfolioNftsRoute = (
  tab: NftDropdownOptionId,
  page?: number,
) => {
  const params = new URLSearchParams({
    tab: tab,
    page: page?.toString() || '1',
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
export const openWalletRouteTab = (route: WalletRoutes | string) => {
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
