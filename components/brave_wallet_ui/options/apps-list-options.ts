// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { AppsListType } from '../constants/types'
import { getLocale } from '../../common/locale'
const compoundIcon = require('../assets/app-icons/compound-icon.png')
const makerIcon = require('../assets/app-icons/maker-icon.jpeg')
const aaveIcon = require('../assets/app-icons/aave-icon.jpeg')
const openSeaIcon = require('../assets/app-icons/opensea-icon.png')
const raribleIcon = require('../assets/app-icons/rarible-icon.png')

export const AppsList = (): AppsListType[] => [
  {
    category: getLocale('braveWalletDefiCategory'),
    categoryButtonText: getLocale('braveWalletDefiButtonText'),
    appList: [
      {
        name: getLocale('braveWalletCompoundName'),
        description: getLocale('braveWalletCompoundDescription'),
        url: '',
        icon: compoundIcon
      },
      {
        name: getLocale('braveWalletMakerName'),
        description: getLocale('braveWalletMakerDescription'),
        url: '',
        icon: makerIcon
      },
      {
        name: getLocale('braveWalletAaveName'),
        description: getLocale('braveWalletAaveDescription'),
        url: '',
        icon: aaveIcon
      }
    ]
  },
  {
    category: getLocale('braveWalletNftCategory'),
    categoryButtonText: getLocale('braveWalletNftButtonText'),
    appList: [
      {
        name: getLocale('braveWalletOpenSeaName'),
        description: getLocale('braveWalletOpenSeaDescription'),
        url: '',
        icon: openSeaIcon
      },
      {
        name: getLocale('braveWalletRaribleName'),
        description: getLocale('braveWalletRaribleDescription'),
        url: '',
        icon: raribleIcon
      }
    ]
  }
]
