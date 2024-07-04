// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import {
  WalletPanelStory //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'
import { EnableTransactionSimulations } from './enable_transaction_simulations'

export const _EnableTransactionSimulations = {
  render: () => {
    return (
      <WalletPanelStory walletApiDataOverrides={{}}>
        <EnableTransactionSimulations />
      </WalletPanelStory>
    )
  }
}

export default { component: EnableTransactionSimulations }
