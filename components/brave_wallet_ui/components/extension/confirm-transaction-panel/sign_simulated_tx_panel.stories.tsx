// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// components
import {
  WalletPanelStory //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'
import { SignSimulatedTransactionPanel } from './sign_simulated_tx_panel'

// styles
import { PanelWrapper } from '../../../panel/style'
import { LongWrapper } from '../../../stories/style'

// mocks
import {
  mockSolanaMainnetNetwork //
} from '../../../stories/mock-data/mock-networks'
import {
  mockAccount,
  mockNoChangeSolSimulation,
  mockReceiveSolSimulation,
  mockSolDappSignTransactionRequest
} from '../../../common/constants/mocks'

export const _SignSimulatedTransactionPanel = {
  title: 'Sign Simulated Transaction Panel',
  render: () => {
    return (
      <WalletPanelStory>
        <PanelWrapper isLonger>
          <LongWrapper>
            <SignSimulatedTransactionPanel
              txSimulation={mockReceiveSolSimulation}
              isSigningDisabled={false}
              network={mockSolanaMainnetNetwork}
              queueLength={1}
              queueNextSignTransaction={() => {}}
              queueNumber={1}
              signSolTransactionsRequest={mockSolDappSignTransactionRequest}
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
              txSimulation={mockNoChangeSolSimulation}
              isSigningDisabled={false}
              network={mockSolanaMainnetNetwork}
              queueLength={1}
              queueNextSignTransaction={() => {}}
              queueNumber={1}
              signSolTransactionsRequest={mockSolDappSignTransactionRequest}
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
