/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import * as style from './button_switch.style'

// ===============
//  CSS Variables
// ===============
//
// --button-switch-padding

export interface ButtonSwitchOption<T> {
  value: T
  content: React.ReactNode
  caption?: React.ReactNode
}

interface Props<T> {
  options: Array<ButtonSwitchOption<T>>
  selectedValue: T
  onSelect: (value: T) => void
}

export function ButtonSwitch<T> (props: Props<T>) {
  return (
    <style.root>
      {
        props.options.map((opt, index) => {
          const key = String(opt.value)
          const selected = opt.value === props.selectedValue
          const onClick = () => {
            if (!selected) {
              props.onSelect(opt.value)
            }
          }
          return (
            <style.option
              key={key}
              className={selected ? 'selected' : ''}
              data-option-value={key}
              data-option-index={index}
            >
              <button onClick={onClick}>{opt.content}</button>
              {opt.caption ? <style.caption>{opt.caption}</style.caption> : null}
            </style.option>
          )
        })
      }
    </style.root>
  )
}
