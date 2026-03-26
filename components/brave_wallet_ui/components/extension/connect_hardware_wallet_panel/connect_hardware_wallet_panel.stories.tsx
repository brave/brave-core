// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { HardwareWalletResponseCodeType } from '../../../constants/types'

// Components
import { ConnectHardwareWalletPanel } from './connect_hardware_wallet_panel'
import {
  WalletPanelStory, //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'

type StorybookConnectHardwareWalletArgs = {
  hardwareWalletCode: HardwareWalletResponseCodeType
}

export const _ConnectHardwareWalletPanel = {
  render: (args: StorybookConnectHardwareWalletArgs) => {
    // Props
    const { hardwareWalletCode } = args
    return (
      <WalletPanelStory>
        <ConnectHardwareWalletPanel hardwareWalletCode={hardwareWalletCode} />
      </WalletPanelStory>
    )
  },
}

export default {
  title: 'Wallet/Panel/Panels',
  component: ConnectHardwareWalletPanel,
  parameters: {
    layout: 'centered',
  },
  argTypes: {
    hardwareWalletCode: {
      options: [
        'deviceNotConnected',
        'deviceBusy',
        'openLedgerApp',
        'transactionRejected',
        'unauthorized',
      ] satisfies HardwareWalletResponseCodeType[],
      control: { type: 'select' },
    },
  },
}
