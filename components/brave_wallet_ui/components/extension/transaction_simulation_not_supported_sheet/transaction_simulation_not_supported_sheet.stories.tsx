// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// components
import {
  WalletPanelStory, //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'
import {
  TransactionSimulationNotSupportedSheet, //
} from './transaction_simulation_not_supported_sheet'

export const _TransactionSimulationNotSupportedSheet = {
  render: () => {
    return (
      <WalletPanelStory>
        <TransactionSimulationNotSupportedSheet />
      </WalletPanelStory>
    )
  },
}

export default {
  title: 'Wallet/Panel/Panels/Transaction Simulations',
  component: TransactionSimulationNotSupportedSheet,
}
