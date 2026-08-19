// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import WalletPageStory from '../../../stories/wrappers/wallet-page-story-wrapper'
import { Deposit } from './deposit'

export const _Deposit = {
  render: () => {
    return (
      <WalletPageStory>
        <Deposit />
      </WalletPageStory>
    )
  },
  name: 'Deposit Funds Screen',
}

export default {
  title: 'Wallet/Desktop/Screens',
  component: Deposit,
}
