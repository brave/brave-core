// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { ApprovalForAllWarning } from './approval_for_all_warning'
import {
  WalletPanelStory, //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'

export const _ApprovalForAllWarning = {
  render: () => {
    return (
      <WalletPanelStory>
        <ApprovalForAllWarning
          onConfirm={() => {}}
          onReject={() => {}}
          address='0x1234567890123456789012345678901234567890'
        />
      </WalletPanelStory>
    )
  },
}

export default {
  title: 'Wallet/Panel/Components/Approval For All Warning',
  component: ApprovalForAllWarning,
  parameters: {
    layout: 'centered',
  },
}
