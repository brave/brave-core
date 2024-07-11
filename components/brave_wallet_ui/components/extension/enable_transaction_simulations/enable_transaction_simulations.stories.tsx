// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// components
import {
  WalletPanelStory //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'
import { EnableTransactionSimulations } from './enable_transaction_simulations'

// styles
import { PanelWrapper } from '../../../panel/style'
import { LongWrapper } from '../../../stories/style'

export const _EnableTransactionSimulations = {
  render: () => {
    return (
      <WalletPanelStory walletApiDataOverrides={{}}>
        <PanelWrapper isLonger={true}>
          <LongWrapper padding='0px'>
            <EnableTransactionSimulations />
          </LongWrapper>
        </PanelWrapper>
      </WalletPanelStory>
    )
  }
}

export default { component: EnableTransactionSimulations }
