// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { EntityId } from '@reduxjs/toolkit'

export interface AccountTokenBalanceForChainId {
  accountEntityId: EntityId
  tokenEntityId: EntityId
  balance: string
  chainId: EntityId
}

export interface TokenBalancesForChainId {
  [contractAddress: string]: string
}
