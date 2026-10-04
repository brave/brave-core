// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Components
import {
  WalletPanelStory, //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'
import { Connections } from './connections'

// Styled Components
import { LongWrapper } from '../../../stories/style'
import { PanelWrapper } from '../../../panel/panel_wrapper/panel_wrapper'

export const _Connections = {
  render: () => {
    return (
      <WalletPanelStory>
        <PanelWrapper>
          <LongWrapper padding='0px'>
            <Connections />
          </LongWrapper>
        </PanelWrapper>
      </WalletPanelStory>
    )
  },
}

export default {
  title: 'Wallet/Panel/Panels',
  component: Connections,
}
