// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Mock Data
import {
  mockTransactionInfo, //
} from '../../../../stories/mock-data/mock-transaction-info'
import { mockEthMainnet } from '../../../../stories/mock-data/mock-networks'

// Utils
import { getLocale } from '../../../../../common/locale'

// Components
import { CustomNetworkFee } from './custom_network_fee'
import {
  WalletPanelStory, //
} from '../../../../stories/wrappers/wallet-panel-story-wrapper'
import { BottomSheet } from '../../../shared/bottom_sheet/bottom_sheet'

export const _CustomNetworkFee = {
  render: () => {
    return (
      <BottomSheet
        isOpen={true}
        title={getLocale('braveWalletCustomFeeAmount')}
        onClose={() => alert('Close Clicked')}
      >
        <CustomNetworkFee
          transactionInfo={mockTransactionInfo}
          selectedNetwork={mockEthMainnet}
          baseFeePerGas='3641000000' // (3.641 gwei)
          onUpdateCustomNetworkFee={() => alert('Update Clicked')}
          onBack={() => alert('Back Clicked')}
          onClose={() => alert('Close Clicked')}
        />
      </BottomSheet>
    )
  },
}

export default {
  title: 'Wallet/Panel/Components/Edit Network Fee',
  component: _CustomNetworkFee,
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
