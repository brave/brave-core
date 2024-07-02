// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import {
  mockAccount,
  mockNoChangeSolSimulation,
  mockReceiveSolSimulation,
  mockSolDappSignTransactionRequest
} from '../../../common/constants/mocks'
import { PanelWrapper } from '../../../panel/style'
import {
  mockSolanaMainnetNetwork //
} from '../../../stories/mock-data/mock-networks'
import { LongWrapper } from '../../../stories/style'

import {
  WalletPanelStory //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'
import { SignSimulatedTransactionPanel } from './sign_simulated_tx_panel'

export const _SignSimulatedTransactionPanel = {
  title: 'Sign Simulated Transaction Panel',
  render: () => {
    return (
      <WalletPanelStory>
        <PanelWrapper isLonger>
          <LongWrapper>
            <SignSimulatedTransactionPanel
              signMode={'signTx'}
              txSimulation={mockReceiveSolSimulation}
              isSigningDisabled={false}
              network={mockSolanaMainnetNetwork}
              queueLength={1}
              queueNextSignTransaction={() => {}}
              queueNumber={1}
              selectedQueueData={mockSolDappSignTransactionRequest}
              signingAccount={mockAccount}
            />
          </LongWrapper>
        </PanelWrapper>
      </WalletPanelStory>
    )
  }
}

export const _EmptySignSimulatedTransactionPanel = {
  title: 'Empty Sign Simulated Transaction Panel',
  render: () => {
    return (
      <WalletPanelStory>
        <PanelWrapper isLonger>
          <LongWrapper>
            <SignSimulatedTransactionPanel
              signMode={'signTx'}
              txSimulation={mockNoChangeSolSimulation}
              isSigningDisabled={false}
              network={mockSolanaMainnetNetwork}
              queueLength={1}
              queueNextSignTransaction={() => {}}
              queueNumber={1}
              selectedQueueData={mockSolDappSignTransactionRequest}
              signingAccount={mockAccount}
            />
          </LongWrapper>
        </PanelWrapper>
      </WalletPanelStory>
    )
  }
}

export default {
  title: 'Sign Simulated Transaction Panel'
}
