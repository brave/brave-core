// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Components
import { CardHub } from './card_hub'
import {
  WalletPanelStory, //
} from '$wallet/stories/wrappers/wallet-panel-story-wrapper'

export const _CardHub = {
  render: () => {
    return <CardHub />
  },
}

export default {
  title: 'Wallet/Panel/Panels/Card Hub',
  component: CardHub,
  parameters: {
    layout: 'centered',
  },
  decorators: [
    (Story: any) => (
      <WalletPanelStory>
        <Story />
      </WalletPanelStory>
    ),
  ],
}
