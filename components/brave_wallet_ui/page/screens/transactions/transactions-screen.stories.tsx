// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import WalletPageStory from '../../../stories/wrappers/wallet-page-story-wrapper'
import TransactionsScreen from './transactions-screen'

export const _TransactionsScreen: React.FC = () => {
  return (
    <WalletPageStory>
      <TransactionsScreen />
    </WalletPageStory>
  )
}

export default _TransactionsScreen
