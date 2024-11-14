/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useAppState } from '../../lib/app_model_context'
import { useBreakpoint } from '../../lib/breakpoint'
import { ContributeCard } from './contribute_card'
import { EarningCard } from './earning_card'
import { PayoutAccountCard } from './payout_account_card'
import { BenefitsCard } from './benefits_card'
import { RecurringContributionCard } from './recurring_contribution_card'

import { style } from './home_view.style'

export function HomeView() {
  const viewType = useBreakpoint()
  const [externalWallet, embedder] = useAppState((state) => [
    state.externalWallet,
    state.embedder
  ])

  if (viewType === 'double' && externalWallet) {
    return (
      <div {...style}>
        <ContributeCard />
        <EarningCard />
        <div className='columns'>
          <div>
            <BenefitsCard />
            <RecurringContributionCard />
          </div>
          <div>
            <PayoutAccountCard />
          </div>
        </div>
      </div>
    )
  }

  return (
    <div {...style}>
      <ContributeCard />
      <EarningCard />
      <PayoutAccountCard />
      <BenefitsCard />
      {
        !embedder.isBubble && <>
          <RecurringContributionCard />
        </>
      }
    </div>
  )
}
