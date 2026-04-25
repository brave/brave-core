// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Components
import {
  WalletPageStory, //
} from '../../../stories/wrappers/wallet-page-story-wrapper'
import { PageNotFound } from './page_not_found'

export const _PageNotFound = {
  render: () => {
    return (
      <WalletPageStory>
        <PageNotFound />
      </WalletPageStory>
    )
  },
}

export default {
  title: 'Wallet/Desktop/Screens',
  component: PageNotFound,
}
