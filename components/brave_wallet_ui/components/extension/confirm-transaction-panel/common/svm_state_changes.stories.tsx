// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// components
import {
  WalletPanelStory //
} from '../../../../stories/wrappers/wallet-panel-story-wrapper'

import {
  SOLTransfer,
  SPLTokenTransfer,
  SolStakingAuthChange
} from './svm_state_changes'

// mocks
import {
  mockSolanaMainnetNetwork //
} from '../../../../stories/mock-data/mock-networks'
import {
  mockReceiveSolSimulation,
  mockSendSolNftEvent,
  mockSendSplTokenEvent,
  mockSolStakingChangeEvent
} from '../../../../common/constants/mocks'

function assertDefined<T>(data: T) {
  if (!data) {
    throw new Error('provided data was undefined, expected data to be defined')
  }

  return data as Exclude<T, undefined>
}

export const _SOLTransfer = () => {
  return (
    <WalletPanelStory>
      <SOLTransfer
        transfer={assertDefined(
          mockReceiveSolSimulation.expectedStateChanges[0].rawInfo.data
            .solTransferData
        )}
        network={mockSolanaMainnetNetwork}
      />
    </WalletPanelStory>
  )
}

export const _SplTransfer = () => {
  return (
    <WalletPanelStory>
      <SPLTokenTransfer
        transfer={assertDefined(
          mockSendSplTokenEvent.rawInfo.data.splTransferData
        )}
        network={mockSolanaMainnetNetwork}
      />
    </WalletPanelStory>
  )
}

export const _SplNftTransfer = () => {
  return (
    <WalletPanelStory>
      <SPLTokenTransfer
        transfer={assertDefined(
          mockSendSolNftEvent.rawInfo.data.splTransferData
        )}
        network={mockSolanaMainnetNetwork}
      />
    </WalletPanelStory>
  )
}

export const _SolStakingChange = () => {
  return (
    <WalletPanelStory>
      <SolStakingAuthChange
        authChange={assertDefined(
          mockSolStakingChangeEvent.rawInfo.data.solStakeAuthorityChangeData
        )}
        network={mockSolanaMainnetNetwork}
      />
    </WalletPanelStory>
  )
}

export default {
  title: 'SVM State Changes'
}
