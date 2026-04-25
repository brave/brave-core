// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Mocks
import {
  mockUniswapOriginInfo,
  mockOriginInfo, //
} from '../../../stories/mock-data/mock-origin-info'

// Components
import { OriginInfoCard } from './origin_info_card'
import {
  WalletPanelStory, //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'

// Styled Components
import { Column } from '../../shared/style'

export const _OriginInfoCardVerified = {
  render: () => {
    return <OriginInfoCard origin={mockUniswapOriginInfo} />
  },
}

export const _OriginInfoCardNotVerified = {
  render: () => {
    return <OriginInfoCard origin={mockOriginInfo} />
  },
}

export default {
  title: 'Wallet/Panel/Components/Origin Info Card',
  component: OriginInfoCard,
  parameters: {
    layout: 'centered',
  },
  decorators: [
    (Story: any) => (
      <WalletPanelStory dontWrapInPanelFrame={true}>
        <Column>
          <Story />
        </Column>
      </WalletPanelStory>
    ),
  ],
}
