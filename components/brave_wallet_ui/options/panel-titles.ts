// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { PanelTitleObjectType } from '../constants/types'
import { getLocale } from '../../common/locale'

export const PanelTitles = (): PanelTitleObjectType[] => [
  {
    id: 'buy',
    title: getLocale('braveWalletBuy')
  },
  {
    id: 'send',
    title: getLocale('braveWalletSend')
  },
  {
    id: 'swap',
    title: getLocale('braveWalletSwap')
  },
  {
    id: 'apps',
    title: getLocale('braveWalletTopTabApps')
  },
  {
    id: 'sitePermissions',
    title: getLocale('braveWalletSitePermissionsTitle')
  },
  {
    id: 'activity', // Transactions
    title: getLocale('braveWalletActivity')
  },
  {
    id: 'transactionDetails',
    title: getLocale('braveWalletTransactionDetails')
  },
  {
    id: 'assets',
    title: getLocale('braveWalletAssetsPanelTitle')
  },
  {
    id: 'currencies',
    title: getLocale('braveWalletSelectCurrency')
  }
]
