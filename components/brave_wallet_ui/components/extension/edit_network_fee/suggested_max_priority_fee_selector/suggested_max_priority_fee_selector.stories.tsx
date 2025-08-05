// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Mock Data
import {
  mockSuggestedMaxPriorityFeeOptions, //
  mockTransactionInfo,
} from '../../../../stories/mock-data/mock-transaction-info'
import { mockEthMainnet } from '../../../../stories/mock-data/mock-networks'

// Utils
import { getLocale } from '../../../../../common/locale'

// Types
import { MaxPriorityFeeTypes } from '../../../../constants/types'

// Components
import {
  SuggestedMaxPriorityFeeSelector, //
} from './suggested_max_priority_fee_selector'
import {
  WalletPanelStory, //
} from '../../../../stories/wrappers/wallet-panel-story-wrapper'
import { BottomSheet } from '../../../shared/bottom_sheet/bottom_sheet'

export const _SuggestedMaxPriorityFeeSelector = {
  render: () => {
    return (
      <BottomSheet
        isOpen={true}
        title={getLocale('braveWalletNetworkFee')}
        onClose={() => alert('Close Clicked')}
      >
        <SuggestedMaxPriorityFeeSelector
          transactionInfo={mockTransactionInfo}
          selectedNetwork={mockEthMainnet}
          baseFeePerGas={'1'}
          suggestedMaxPriorityFee='average'
          suggestedMaxPriorityFeeOptions={mockSuggestedMaxPriorityFeeOptions}
          onClickCustom={() => alert('Custom Clicked')}
          setSuggestedMaxPriorityFee={(value: MaxPriorityFeeTypes) =>
            alert(`Updated to ${value}`)
          }
        />
      </BottomSheet>
    )
  },
}

export default {
  title: 'Wallet/Panel/Components/Edit Network Fee',
  component: SuggestedMaxPriorityFeeSelector,
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
