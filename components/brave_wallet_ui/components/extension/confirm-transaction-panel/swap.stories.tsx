// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Mocks
import {
  mockBasicAttentionToken,
  mockBinanceCoinErc20Token
} from '../../../stories/mock-data/mock-asset-options'

// Components
import WalletPanelStory from '../../../stories/wrappers/wallet-panel-story-wrapper'
import { LongWrapper } from '../../../stories/style'
import { PanelWrapper } from '../../../panel/style'
import { ConfirmSwapTransaction } from './swap'

export const _ConfirmSwapTransaction = () => {
  return (
    <WalletPanelStory
      walletStateOverride={{
        fullTokenList: [mockBasicAttentionToken, mockBinanceCoinErc20Token],
        hasInitialized: true,
        isWalletCreated: true
      }}
      panelStateOverride={{
        hasInitialized: true
      }}
    >
      <PanelWrapper isLonger={true}>
        <LongWrapper>
          <ConfirmSwapTransaction />
        </LongWrapper>
      </PanelWrapper>
    </WalletPanelStory>
  )
}

_ConfirmSwapTransaction.story = {
  name: 'Confirm Swap'
}

export default _ConfirmSwapTransaction
