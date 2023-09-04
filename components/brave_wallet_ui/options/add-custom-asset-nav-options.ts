// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { getLocale } from '$web-common/locale'
import { TabOption } from '../components/shared/tabs/tabs'

export const CUSTOM_ASSET_NAV_OPTIONS: TabOption[] = [
  {
    id: 'token',
    label: getLocale('braveWalletAddAssetTokenTabTitle')
  },
  {
    id: 'nft',
    label: getLocale('braveWalletAddAssetNftTabTitle')
  }
]
