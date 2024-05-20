// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import {
  WalletPanelStory //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'
import { CopyLabel } from './copy_label'

export const _CopyLabel = () => {
  return (
    <WalletPanelStory>
      <CopyLabel textToCopy='text to copy'>
        Text <strong>Bold</strong>
      </CopyLabel>
    </WalletPanelStory>
  )
}

_CopyLabel.storyName = 'Copy Label'

export default _CopyLabel
