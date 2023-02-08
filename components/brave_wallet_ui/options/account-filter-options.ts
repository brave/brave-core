// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { WalletAccountType } from '../constants/types'
import { getLocale } from '../../common/locale'

export const AllAccountsOption: WalletAccountType = {
  accountType: 'Primary',
  address: 'all',
  coin: 0,
  id: 'all',
  name: getLocale('braveWalletAccountFilterAllAccounts'),
  nativeBalanceRegistry: {},
  tokenBalanceRegistry: {},
  keyringId: ''
}
