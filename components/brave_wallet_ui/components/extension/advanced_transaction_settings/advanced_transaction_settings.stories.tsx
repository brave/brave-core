// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { getLocale } from '../../../../common/locale'

// Components
import {
  AdvancedTransactionSettings, //
} from './advanced_transaction_settings'

// Wrappers
import {
  WalletPanelStory, //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'
import { BottomSheet } from '../../shared/bottom_sheet/bottom_sheet'

export const _AdvancedTransactionSettings = {
  render: () => {
    return (
      <BottomSheet
        isOpen={true}
        title={getLocale('braveWalletAdvancedTransactionSettings')}
        onClose={() => alert('Close Clicked')}
      >
        <AdvancedTransactionSettings
          onCancel={() => alert('Cancel Clicked')}
          nonce='1'
          onSave={(nonce: string) => alert(`Nonce updated to ${nonce}`)}
        />
      </BottomSheet>
    )
  },
}

export default {
  title: 'Wallet/Panel/Components/Advanced Transaction Settings',
  component: AdvancedTransactionSettings,
  parameters: {
    layout: 'centered',
  },
  decorators: [
    (Story: any) => (
      <WalletPanelStory>
        <Story />
      </WalletPanelStory>
    ),
  ],
}
