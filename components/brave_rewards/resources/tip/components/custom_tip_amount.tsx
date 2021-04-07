/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import * as style from './custom_tip_amount.style'

interface Props {
  text: React.ReactNode
  amount: number
  currency: React.ReactNode
  exchangeAmount: React.ReactNode
  onReset: () => void
}

export function CustomTipAmount (props: Props) {
  return (
    <style.root>
      <style.text>{props.text}</style.text>
      <style.amount>
        {props.amount}&nbsp;
        <style.currency>{props.currency}</style.currency>
        <style.reset>
          <button onClick={props.onReset}>
            <div className='icon'>+</div>
          </button>
        </style.reset>
      </style.amount>
      <style.exchange>
        {props.exchangeAmount}
      </style.exchange>
    </style.root>
  )
}
