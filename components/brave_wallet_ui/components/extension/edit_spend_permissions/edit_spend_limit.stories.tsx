// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { getLocale } from '../../../../common/locale'

// Components
import {
  EditSpendLimit, //
} from './edit_spend_limit'
import { BottomSheet } from '../../shared/bottom_sheet/bottom_sheet'
import {
  WalletPanelStory, //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'

export const _EditSpendLimit = {
  render: () => {
    return (
      <BottomSheet
        isOpen={true}
        title={getLocale('braveWalletEditPermissionsTitle')}
        onClose={() => alert('Close Clicked')}
      >
        <EditSpendLimit
          onCancel={() => alert('Cancel Clicked')}
          onSave={(limit: string) => alert(`Spend limit updated to ${limit}`)}
          approvalTarget='0x Exchange Proxy'
          isApprovalUnlimited={false}
          proposedAllowance='100'
          symbol='ETH'
        />
      </BottomSheet>
    )
  },
}

export default {
  title: 'Wallet/Panel/Components/Edit Spend Limit',
  component: EditSpendLimit,
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
