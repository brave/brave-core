// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import {
  WalletPanelStory //
} from '../../../../stories/wrappers/wallet-panel-story-wrapper'

import { PaymentMethodGroup } from './payment-method-group'
import { PaymentMethodOptionNames } from '../../../../constants/types'

export const _PaymentMethodGroup = () => {
  return (
    <WalletPanelStory>
      <PaymentMethodGroup
        options={[...PaymentMethodOptionNames]}
        type='bank'
      />
    </WalletPanelStory>
  )
}

_PaymentMethodGroup.storyName = 'Payment Method Group'

export default _PaymentMethodGroup
