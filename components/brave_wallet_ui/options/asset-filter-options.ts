// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { AssetFilterOption } from '../constants/types'
import { getLocale } from '../../common/locale'

export const HighToLowAssetsFilterOption: AssetFilterOption = {
  id: 'highToLow',
  name: getLocale('braveWalletAssetFilterHighToLow')
}

export const AssetFilterOptions: AssetFilterOption[] = [
  HighToLowAssetsFilterOption,
  {
    id: 'lowToHigh',
    name: getLocale('braveWalletAssetFilterLowToHigh')
  },
  {
    id: 'aToZ',
    name: getLocale('braveWalletAssetFilterAToZ')
  },
  {
    id: 'zToA',
    name: getLocale('braveWalletAssetFilterZToA')
  }
]
