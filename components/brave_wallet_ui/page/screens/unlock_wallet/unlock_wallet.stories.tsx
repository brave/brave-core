// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import {
  WalletPageStory, //
} from '../../../stories/wrappers/wallet-page-story-wrapper'
import { UnlockWallet } from './unlock_wallet'

export const _UnlockWallet = {
  render: () => {
    return (
      <WalletPageStory>
        <UnlockWallet />
      </WalletPageStory>
    )
  },
}

export default {
  title: 'Wallet/Desktop/Screens',
  component: UnlockWallet,
}
