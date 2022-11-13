// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// mocks
import { mockAccount, mockNetwork } from '../../../common/constants/mocks'

// components
import WalletPanelStory from '../../../stories/wrappers/wallet-panel-story-wrapper'
import TransactionsPanel from '.'
import { Column } from '../../shared/style'

export const _TransactionsPanel: React.FC = () => {
  return (
    <WalletPanelStory>
      <Column
        color={
          '#F8F9FA' // light
          // '#17171F' // dark
        }
      >
        <TransactionsPanel
          onSelectTransaction={() => null}
          selectedAccountAddress={mockAccount.address}
          selectedNetwork={mockNetwork}
        />
      </Column>
    </WalletPanelStory>
  )
}

export default _TransactionsPanel
