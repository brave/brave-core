// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { PanelTitleObjectType } from '../constants/types'

export const PANEL_TITLES: PanelTitleObjectType[] = [
  {
    id: 'buy',
    title: 'braveWalletBuy'
  },
  {
    id: 'send',
    title: 'braveWalletSend'
  },
  {
    id: 'swap',
    title: 'braveWalletSwap'
  },
  {
    id: 'sitePermissions',
    title: 'braveWalletSitePermissionsTitle'
  },
  {
    id: 'activity', // Transactions
    title: 'braveWalletActivity'
  },
  {
    id: 'transactionDetails',
    title: 'braveWalletTransactionDetails'
  },
  {
    id: 'assets',
    title: 'braveWalletAssetsPanelTitle'
  }
]
