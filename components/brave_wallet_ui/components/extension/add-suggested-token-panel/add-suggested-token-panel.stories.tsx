// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import AddSuggestedTokenPanel from '.'
import { PanelWrapper } from '../../../panel/style'
import WalletPanelStory from '../../../stories/wrappers/wallet-panel-story-wrapper'

export const _AddSuggestedTokenPanel = {
  render: () => {
    return (
      <WalletPanelStory>
        <PanelWrapper>
          <AddSuggestedTokenPanel />
        </PanelWrapper>
      </WalletPanelStory>
    )
  }
}

export default { component: AddSuggestedTokenPanel }
