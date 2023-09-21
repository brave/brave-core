// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { ExploreOverview } from './explore-overview'
import WalletPageStory from '../../../../stories/wrappers/wallet-page-story-wrapper'

export const _ExploreOverview = () => {
  return (
    <WalletPageStory>
      <ExploreOverview />
    </WalletPageStory>
  ) 
}

export default _ExploreOverview.storyName = 'Explore Overview'
