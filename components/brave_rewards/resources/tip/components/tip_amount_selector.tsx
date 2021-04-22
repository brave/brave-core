/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { ButtonSwitch } from './button_switch'

import * as style from './tip_amount_selector.style'

export interface TipAmountOption {
  value: number
  amount?: React.ReactNode
  currency: React.ReactNode
  exchangeAmount: React.ReactNode
}

interface Props {
  selectedValue: number
  options: TipAmountOption[]
  onSelect: (amount: number) => void
}

export function TipAmountSelector (props: Props) {
  const options = props.options.map((item) => {
    const amount = item.amount || String(item.value)
    return {
      value: item.value,
      content: item.currency
        ? <><strong>{amount}</strong> {item.currency}</>
        : <><strong>{amount}</strong></>,
      caption: item.exchangeAmount || undefined
    }
  })

  return (
    <style.root>
      <style.amounts data-test-id='tip-amount-options'>
        <ButtonSwitch<number>
          options={options}
          selectedValue={props.selectedValue}
          onSelect={props.onSelect}
        />
      </style.amounts>
    </style.root>
  )
}
