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

export const _ConnectHardwareWalletPanel = {
  render: () => {
    return (
      <WalletPanelStory>
        <PanelWrapper>
          <ConnectHardwareWalletPanel hardwareWalletCode={undefined} />
        </PanelWrapper>
      </WalletPanelStory>
    )
  }
}

export default { component: ConnectHardwareWalletPanel }
