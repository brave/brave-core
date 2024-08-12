/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { EarningCard } from './earning_card'
import { PayoutAccountCard } from './payout_account_card'

import { style } from './home_view.style'

export function HomeView() {
  return (
    <div {...style}>
      <EarningCard />
      <PayoutAccountCard />
    </div>
  )
}
