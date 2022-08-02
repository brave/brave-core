// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { AssetFilterOption } from '../constants/types'
import { getLocale } from '../../common/locale'

export const AllAssetsFilterOption: AssetFilterOption = {
  id: 'allAssets',
  name: getLocale('braveWalletAssetFilterAllAssets')
}

export const AssetFilterOptions: AssetFilterOption[] = [
  AllAssetsFilterOption,
  {
    id: 'nfts',
    name: getLocale('braveWalletAssetFilterNFTs')
  },
  {
    id: 'lowToHigh',
    name: getLocale('braveWalletAssetFilterLowToHigh')
  },
  {
    id: 'highToLow',
    name: getLocale('braveWalletAssetFilterHighToLow')
  }
]
