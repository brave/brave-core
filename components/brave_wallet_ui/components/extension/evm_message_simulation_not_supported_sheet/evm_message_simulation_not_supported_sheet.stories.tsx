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
  EvmMessageSimulationNotSupportedSheet //
} from './evm_message_simulation_not_supported_sheet'

export const _EvmMessageSimulationNotSupportedSheet = {
  title: 'EVM Message Simulation Not Supported Sheet',
  render: () => {
    return (
      <WalletPanelStory>
        <EvmMessageSimulationNotSupportedSheet />
      </WalletPanelStory>
    )
  }
}

export default {
  title: 'EVM Message Simulation Not Supported Sheet',
  component: EvmMessageSimulationNotSupportedSheet
}
