// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// components
import {
  WalletPanelStory //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'
import {
  TransactionSimulationNotSupportedSheet //
} from './transaction_simulation_not_supported_sheet'

// mocks
import { mockNetwork } from '../../../common/constants/mocks'

export const _TransactionSimulationNotSupportedSheet = () => {
  return (
    <WalletPanelStory>
      <TransactionSimulationNotSupportedSheet network={mockNetwork} />
    </WalletPanelStory>
  )
}

_TransactionSimulationNotSupportedSheet.storyName =
  'Transaction Simulation Not Supported Sheet'

export default _TransactionSimulationNotSupportedSheet
