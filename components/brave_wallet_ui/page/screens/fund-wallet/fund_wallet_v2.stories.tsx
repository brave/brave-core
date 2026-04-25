// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Components
import {
  WalletPageStory, //
} from '../../../stories/wrappers/wallet-page-story-wrapper'
import { FundWalletScreen } from './fund_wallet_v2'

export const _FundWalletScreenV2 = () => {
  return (
    <WalletPageStory>
      <FundWalletScreen />
    </WalletPageStory>
  )
}

export default {
  component: FundWalletScreen,
  title: 'Wallet/Desktop/Screens',
}
