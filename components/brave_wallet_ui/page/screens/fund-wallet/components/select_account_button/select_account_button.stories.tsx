// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { SelectAccountButton } from './select_account_button'
import WalletPageStory from '../../../../../stories/wrappers/wallet-page-story-wrapper'
import { mockAccount } from '../../../../../common/constants/mocks'

export const _SelectAccountButton = () => {
  return (
    <WalletPageStory>
      <SelectAccountButton
        labelText='Account'
        selectedAccount={{...mockAccount, name: 'My ethereum account'}}
        onClick={() => console.log('Open account selection modal')}
      />
    </WalletPageStory>
  )
}

_SelectAccountButton.story = {
  name: 'Fund Wallet - Select Account Button'
}

export default _SelectAccountButton
