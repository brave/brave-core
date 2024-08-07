// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Components
import {
  WalletPageStory //
} from '../../../../stories/wrappers/wallet-page-story-wrapper'
import { RemoveAccountModal } from './remove-account-modal'

// Mocks
import { mockAccount } from '../../../../common/constants/mocks'

export const _RemoveAccountModal = {}
export default {
  title: 'Remove Account Modal',
  render: () => (
    <div style={{ width: '100%', height: '100%' }}>
      <WalletPageStory
        accountTabStateOverride={{
          accountToRemove: { ...mockAccount, name: 'BTC Import' }
        }}
      >
        <RemoveAccountModal />
      </WalletPageStory>
    </div>
  )
}
