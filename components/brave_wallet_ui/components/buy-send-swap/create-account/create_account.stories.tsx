// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import {
  WalletPanelStory //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'
import { CreateAccountTab } from '.'

import { mockNetworks } from '../../../stories/mock-data/mock-networks'

export const _CreateAccount = () => {
  return (
    <WalletPanelStory>
      <CreateAccountTab
        network={mockNetworks[0]}
        onCancel={() => {}}
      />
    </WalletPanelStory>
  )
}

_CreateAccount.storyName = 'Create Account Tab'

export default _CreateAccount
