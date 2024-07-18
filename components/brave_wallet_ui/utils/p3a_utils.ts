// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// types
import type {
  TokenBalancesRegistry //
} from '../common/slices/entities/token-balance.entity'

// constants
import { BraveWallet } from '../constants/types'

// utils
import { loadTimeData } from '../../common/loadTimeData'
import { getActiveWalletCount } from './balance-utils'
import getAPIProxy from '../common/async/bridge'

export async function reportActiveWalletsToP3A(
  accountIds: BraveWallet.AccountId[],
  tokenBalancesRegistry: TokenBalancesRegistry
) {
  const countTestNetworks = loadTimeData.getBoolean(
    BraveWallet.P3A_COUNT_TEST_NETWORKS_LOAD_TIME_KEY
  )

  const activeWalletCount = getActiveWalletCount(
    accountIds,
    tokenBalancesRegistry,
    countTestNetworks
  )

  const { braveWalletP3A } = getAPIProxy()

  for (const [coin, count] of Object.entries(activeWalletCount)) {
    braveWalletP3A.recordActiveWalletCount(count, parseInt(coin))
  }
}
