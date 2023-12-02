// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import {
  WalletPanelStory //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'
import ConnectHardwareWalletPanel from '.'
import { PanelWrapper } from '../../../panel/style'

import { mockAccounts } from '../../../stories/mock-data/mock-wallet-accounts'

export const _ConnectHardwareWalletPanel = () => {
  return (
    <WalletPanelStory>
      <PanelWrapper>
        <ConnectHardwareWalletPanel
          account={{ ...mockAccounts[0], name: 'Ledger 1' }}
          hardwareWalletCode={undefined}
        />
      </PanelWrapper>
    </WalletPanelStory>
  )
}

_ConnectHardwareWalletPanel.storyName = 'Connect Hardware Wallet Panel'

export default _ConnectHardwareWalletPanel
