// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// types
import {
  PaymentMethodOptionNames,
  PaymentProviderType
} from '../../../../constants/types'

// styles
import {
  PaymentMethodGroupCollapse,
  PaymentMethodGroupIcon,
  PaymentMethodGroupSubTitle,
  PaymentMethodGroupTitle
} from './payment-method-group.styles'
import { Column, Row } from '../../../../components/shared/style'

// TODO: locale
const paymentMethodProviderTitles = {
  'bank': 'Bank',
  'card': 'Card',
  'exchange': 'Exchange',
  'mobile-wallet': 'Mobile Wallet'
} as const

export const PaymentMethodGroup = ({
  type,
  options
}: {
  type: PaymentProviderType
  options: PaymentMethodOptionNames[]
}) => {
  // state
  const [isOpen, setIsOpen] = React.useState(false)

  // render
  return (
    <PaymentMethodGroupCollapse
      isOpen={isOpen}
      onToggle={() => setIsOpen((prev) => !prev)}
    >
      <div slot='icon'>
        <PaymentMethodGroupIcon type={type} />
      </div>

      <div slot='title'>
        <Column
          alignItems='flex-start'
          gap={'4px'}
          padding={'0px 0px 0px 16px'}
        >
          <PaymentMethodGroupTitle>
            {paymentMethodProviderTitles[type]}
            {/* TODO: locale & pluralization */}
          </PaymentMethodGroupTitle>
          <PaymentMethodGroupSubTitle>
            {`${options.length} available methods`}
          </PaymentMethodGroupSubTitle>
        </Column>
      </div>

      {options.map((opt) => (
        <Row justifyContent='flex-start'>{opt}</Row>
      ))}
    </PaymentMethodGroupCollapse>
  )
}

export default PaymentMethodGroup
