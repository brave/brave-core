// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Mock Data
import { mockAccount } from '../../../../../common/constants/mocks'

// Components
import { SelectAccountButton } from './select_account_button'
import {
  WalletPageStory //
} from '../../../../../stories/wrappers/wallet-page-story-wrapper'

export const _SelectAccountButton = () => {
  return (
    <WalletPageStory>
      <SelectAccountButton
        labelText='Account'
        selectedAccount={{ ...mockAccount, name: 'My ethereum account' }}
        onClick={() => console.log('Open account selection modal')}
      />
    </WalletPageStory>
  )
}

export default {
  component: _SelectAccountButton,
  title: 'Fund Wallet - Select Account Button'
}
