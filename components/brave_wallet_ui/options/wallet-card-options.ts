// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Types
import { WalletCardOption } from '../constants/types'

export const WalletCardOptions: WalletCardOption[] = [
  {
    id: 'crypto',
    label: S.BRAVE_WALLET_CRYPTO,
    icon: 'product-brave-wallet',
  },
  {
    id: 'brave-rewards',
    label: S.BRAVE_WALLET_BRAVE_REWARDS_TITLE,
    icon: 'coins',
  },
  {
    id: 'brave-rewards-card',
    label: S.BRAVE_WALLET_BRAVE_REWARDS_CARD,
    icon: 'credit-card',
  },
]
