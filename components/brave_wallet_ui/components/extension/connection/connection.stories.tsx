// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Components
import {
  WalletPanelStory //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'
import { Connection } from './connection'

// Styled Components
import { LongWrapper } from '../../../stories/style'
import { PanelWrapper } from '../../../panel/style'

export const _Connection = {
  render: () => {
    return (
      <WalletPanelStory>
        <PanelWrapper
          width={390}
          height={650}
        >
          <LongWrapper padding='0px'>
            <Connection />
          </LongWrapper>
        </PanelWrapper>
      </WalletPanelStory>
    )
  }
}

export default {
  component: Connection
}
