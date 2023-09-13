// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { AccountPageTabs, BraveWallet, WalletRoutes } from '../constants/types'

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
    ) + '#' + transactionId.replace('#', '')
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
