/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import * as style from './slider_switch.style'

// ===============
//  CSS Variables
// ===============
//
// --slider-switch-selected-color

export interface SliderSwitchOption<T> {
  value: T
  content: React.ReactNode
}

interface Props<T> {
  options: Array<SliderSwitchOption<T>>
  selectedValue: T
  onSelect: (value: T) => void
}

export function SliderSwitch<T> (props: Props<T>) {
  const optionCount = props.options.length
  const selectedIndex = props.options.findIndex(
    (option) => option.value === props.selectedValue)

  return (
    <style.root>
      {
        selectedIndex >= 0
          ? <style.bar
            style={{
              left: `${selectedIndex * 100 / optionCount}%`,
              right: `${(optionCount - selectedIndex - 1) * 100 / optionCount}%`
            }}
          /> : null
      }
      <style.rail>
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
                <button onClick={onClick}>
                  <span>{opt.content}</span>
                </button>
              </style.option>
            )
          })
        }
      </style.rail>
    </style.root>
  )
}
