// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import {
  WalletPanelStory //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'
import { PanelWrapper } from '../../../panel/style'
import { LongWrapper } from '../../../stories/style'
import { SignCowSwapOrder } from './cow_swap_order'

import { mockSignMessageRequest } from '../../../stories/mock-data/mock-eth-requests'

export const _SignCowSwapOrder = {
  render: () => {
    return (
      <WalletPanelStory>
        <PanelWrapper isLonger>
          <LongWrapper>
            <SignCowSwapOrder
              data={mockSignMessageRequest}
              onCancel={() => { }}
              onSignIn={() => { }}
              onQueueNextSignMessage={() => { }}
              queueLength={1}
              queueNumber={0}
              isDisabled={false}
            />
          </LongWrapper>
        </PanelWrapper>
      </WalletPanelStory>
    )
  }
}

export default { component: SignCowSwapOrder }
