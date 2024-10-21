// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Components
import {
  WalletPageStory //
} from '../../../stories/wrappers/wallet-page-story-wrapper'
import { FundWalletScreen } from './fund_wallet_v2'

export const _FundWalletScreen = () => {
  return (
    <WalletPageStory>
      <FundWalletScreen />
    </WalletPageStory>
  )
}

_FundWalletScreen.story = {
  name: 'Fund Wallet Screen v2'
}

export default {
  component: _FundWalletScreen,
  title: 'Fund Wallet Screen v2'
}
