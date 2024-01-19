// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import Collapse from '@brave/leo/react/collapse'
import Icon from '@brave/leo/react/icon'
import * as leo from '@brave/leo/tokens/css'

// types
import { PaymentProviderType } from '../../../../constants/types'

export const PaymentMethodGroupCollapse = styled(Collapse)`
  --leo-collapse-summary-padding: 24px;
`

export const PaymentMethodGroupTitle = styled.span`
  color: ${leo.color.text.primary};
  font: ${leo.font.heading.h4};
`

export const PaymentMethodGroupSubTitle = styled.span`
  color: ${leo.color.text.tertiary};
  font: ${leo.font.default.regular};
`

const paymentMethodGroupIconNames = {
  'bank': 'bank',
  'card': 'credit-card',
  'exchange': 'currency-exchange',
  'mobile-wallet': 'product-brave-wallet'
} as const

interface GroupIconProps {
  type: PaymentProviderType
}

export const PaymentMethodGroupIcon = styled(Icon).attrs<GroupIconProps>(
  ({ type }) => ({
    name: paymentMethodGroupIconNames[type]
  })
)<GroupIconProps>`
  width: 24;
  height: 24;
`
