// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import WalletPageStory from '../../../../stories/wrappers/wallet-page-story-wrapper'
import { ExploreDapps } from './components/explore-dapps'

export const _ExploreDapps = () => {
  return (
    <WalletPageStory>
      <ExploreDapps />
    </WalletPageStory>
  ) 
}

export default _ExploreDapps.storyName = 'Explore Dapps'
