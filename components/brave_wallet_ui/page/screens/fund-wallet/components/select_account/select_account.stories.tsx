// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { SelectAccount } from './select_account'
import WalletPageStory from '../../../../../stories/wrappers/wallet-page-story-wrapper'
import { mockAccounts } from '../../../../../stories/mock-data/mock-wallet-accounts'

export const _SelectAccount = () => {
  return (
    <WalletPageStory>
      <SelectAccount
        isOpen={true}
        accounts={mockAccounts}
        onSelect={(account) => console.log(account)}
      />
    </WalletPageStory>
  )
}

export default {
  component: _SelectAccount,
  title: 'Fund Wallet - Select Account'
}
