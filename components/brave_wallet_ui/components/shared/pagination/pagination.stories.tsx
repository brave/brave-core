// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import {
  WalletPanelStory //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'
import { Pagination } from './pagination'

export const _Pagination = {
  render: () => {
    // state
    const [currentPageNumber, onSelectPageNumber] = React.useState(1)

    // render
    return (
      <WalletPanelStory>
        <Pagination
          currentPageNumber={currentPageNumber}
          onSelectPageNumber={onSelectPageNumber}
          lastPageNumber={99999}
        />
      </WalletPanelStory>
    )
  }
}

export default { component: Pagination }
