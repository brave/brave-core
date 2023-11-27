// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import {
  mockAccount,
  mockReceiveSolSimulation,
  mockSolDappSignTransactionRequest
} from '../../../common/constants/mocks'
import { mockSolanaMainnetNetwork } from '../../../stories/mock-data/mock-networks'

import {
  WalletPanelStory //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'
import { SignSimulatedTransactionPanel } from './sign_simulated_tx_panel'

export const _SignSimulatedTransactionPanel = () => {
  return (
    <WalletPanelStory>
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
    </WalletPanelStory>
  )
}

_SignSimulatedTransactionPanel.storyName = 'Sign Simulated Transaction Panel'

export default _SignSimulatedTransactionPanel
