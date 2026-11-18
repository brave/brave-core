// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Components
import { GetStarted } from './get_started'
import { WalletPanelStory } from '$wallet/stories/wrappers/wallet-panel-story-wrapper'

export const _GetStarted = {
  render: () => {
    return (
      <WalletPanelStory>
        <GetStarted />
      </WalletPanelStory>
    )
  },
}

export default {
  title: 'Wallet/Panel/Panels/Card Hub/Get Started',
  component: GetStarted,
  parameters: {
    layout: 'centered',
  },
}
