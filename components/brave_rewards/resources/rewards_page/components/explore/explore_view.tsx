/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import ProgressRing from '@brave/leo/react/progressRing'

import { useAppState } from '../../lib/app_model_context'
import { useBreakpoint } from '../../lib/breakpoint'
import { CommunityCard } from './community_card'
import { MerchStoreCard } from './merch_store_card'

import { style } from './explore_view.style'

export function ExploreView() {
  const viewType = useBreakpoint()
  const [cards] = useAppState((state) => [state.cards])

  if (!cards) {
    return (
      <div {...style}>
        <div className='loading'>
          <ProgressRing />
        </div>
      </div>
    )
  }

  if (viewType === 'double') {
    return (
      <div {...style}>
        <div className='columns'>
          <div>
            <MerchStoreCard />
          </div>
          <div>
            <CommunityCard />
          </div>
        </div>
      </div>
    )
  }

  return (
    <div {...style}>
      <MerchStoreCard />
      <CommunityCard />
    </div>
  )
}
