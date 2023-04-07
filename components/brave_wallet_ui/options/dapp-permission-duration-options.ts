// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Types
import { BraveWallet, DAppPermissionDurationOption } from '../constants/types'

export const DAppPermissionDurationOptions: DAppPermissionDurationOption[] = [
  {
    name: 'braveWalletPermissionUntilClose',
    id: BraveWallet.PermissionLifetimeOption.kPageClosed
  },
  {
    name: 'braveWalletPermissionOneDay',
    id: BraveWallet.PermissionLifetimeOption.k24Hours
  },
  {
    name: 'braveWalletPermissionOneWeek',
    id: BraveWallet.PermissionLifetimeOption.k7Days
  },
  {
    name: 'braveWalletPermissionForever',
    id: BraveWallet.PermissionLifetimeOption.kForever
  }
]
