// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { AccountPageTabs, BraveWallet, WalletRoutes } from '../constants/types'
import { LOCAL_STORAGE_KEYS } from '../common/constants/local-storage-keys'

/**
 * Checks the provided route against a list of routes that we are OK with the
 * wallet opening to when the app is unlocked or when the panel is re-opened
 */
export function isPersistableSessionRoute(route?: string) {
  if (!route) {
    return
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
    route.includes(WalletRoutes.SendPageStart) ||
    route.includes(WalletRoutes.LocalIpfsNode) ||
    route.includes(WalletRoutes.InspectNfts)
  )
}

export function getInitialSessionRoute(): string | undefined {
  const route =
    window.localStorage.getItem(LOCAL_STORAGE_KEYS.SESSION_ROUTE) || ''
  return isPersistableSessionRoute(route) ? route : undefined
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
  currencyCode?: string,
  buyAmount?: string,
  searchText?: string,
  chainId?: string,
  coinType?: string
) => {
  const routePartial = WalletRoutes.FundWalletPage.replace(
    '/:currencyCode?',
    currencyCode ? `/${currencyCode}` : ''
  ).replace('/:buyAmount?', currencyCode && buyAmount ? `/${buyAmount}` : '')

  const params = new URLSearchParams()
  if (searchText) {
    params?.append('search', searchText)
  }
  if (chainId) {
    params?.append('chainId', chainId)
  }
  if (coinType) {
    params?.append('coinType', coinType)
  }

  const paramsString = params ? params.toString() : undefined

  return `${routePartial}${paramsString ? `?${paramsString}` : ''}`
}

export const makeFundWalletPurchaseOptionsRoute = (
  currencyCode: string,
  buyAmount: string
) => {
  return WalletRoutes.FundWalletPurchaseOptionsPage.replace(
    ':currencyCode',
    currencyCode
  ).replace(':buyAmount', buyAmount)
}

export const makeDepositFundsRoute = (
  searchText?: string,
  chainId?: string,
  coinType?: string
) => {
  const params = new URLSearchParams()
  if (searchText) {
    params?.append('search', searchText)
  }
  if (chainId) {
    params?.append('chainId', chainId)
  }
  if (coinType) {
    params?.append('coinType', coinType)
  }

  const paramsString = params ? params.toString() : undefined

  return `${WalletRoutes.DepositFundsPage}${
    paramsString ? `?${paramsString}` : ''
  }`
}
