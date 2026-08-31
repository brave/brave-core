// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Types
import { BraveWallet, DAppPermissionDurationOption } from '../constants/types'

export const DAppPermissionDurationOptions: DAppPermissionDurationOption[] = [
  {
    name: S.BRAVE_WALLET_PERMISSION_UNTIL_CLOSE,
    id: BraveWallet.PermissionLifetimeOption.kPageClosed,
  },
  {
    name: S.BRAVE_WALLET_PERMISSION_ONE_DAY,
    id: BraveWallet.PermissionLifetimeOption.k24Hours,
  },
  {
    name: S.BRAVE_WALLET_PERMISSION_ONE_WEEK,
    id: BraveWallet.PermissionLifetimeOption.k7Days,
  },
  {
    name: S.BRAVE_WALLET_PERMISSION_FOREVER,
    id: BraveWallet.PermissionLifetimeOption.kForever,
  },
]
