// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { PanelWrapper } from '../../../panel/style'
import { LongWrapper } from '../../../stories/style'
import WalletPanelStory from '../../../stories/wrappers/wallet-panel-story-wrapper'

import { ConfirmTransactionPanel } from './confirm-transaction-panel'

export const _ConfirmTransactionPanel = {
  render: () => {
    return (
      <WalletPanelStory>
        <PanelWrapper>
          <LongWrapper>
            <ConfirmTransactionPanel retrySimulation={() => {}} />
          </LongWrapper>
        </PanelWrapper>
      </WalletPanelStory>
    )
  }
}

export default { component: ConfirmTransactionPanel }
