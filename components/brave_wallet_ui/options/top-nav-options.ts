// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { TopTabNavObjectType } from '../constants/types'
import { getLocale } from '$web-common/locale'

export const TopNavOptions = (): TopTabNavObjectType[] => [
  {
    id: 'portfolio',
    name: getLocale('braveWalletTopNavPortfolio')
  },
  {
    id: 'market',
    name: getLocale('braveWalletTopNavMarket')
  },
  {
    id: 'nfts',
    name: getLocale('braveWalletTopNavNFTS')
  },
  {
    id: 'accounts',
    name: getLocale('braveWalletTopNavAccounts')
  }
  // Temp commented out for MVP
  // {
  //   id: 'apps',
  //   name: getLocale('braveWalletTopTabApps')
  // }
]

export const TOP_NAV_OPTIONS = TopNavOptions()
