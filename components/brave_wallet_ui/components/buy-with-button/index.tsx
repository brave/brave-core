/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import {
  StyledWrapper,
  Button,
  CaratDown,
  Dropdown,
  Option
} from './style'
import { BraveWallet, BuyOption } from '../../constants/types'
import { RampLogo } from '../buy-send-swap/buy/style'

export interface Props {
  children?: React.ReactNode
  options: BuyOption[]
  value: BraveWallet.OnRampProvider
  closeOnSelect?: boolean
  disabled?: boolean
  onSelect: (value: BraveWallet.OnRampProvider) => void
}

const SelectBuy = (props: Props) => {
  const { children, options, value, closeOnSelect, disabled, onSelect } = props
  const [isOpen, setIsOpen] = React.useState(false)

  const onClick = () => {
    setIsOpen(prevIsOpen => !prevIsOpen)
  }

  const onOptionSelect = (value: BraveWallet.OnRampProvider) => {
    if (closeOnSelect) {
      setIsOpen(false)
    }

    onSelect(value)
  }

  return (
    <StyledWrapper>
      <Button onClick={onClick} disabled={disabled}>
        {children}
        {!disabled && <CaratDown/>}
      </Button>
      {!disabled && isOpen &&
          <Dropdown>
            {options.map(option =>
              <Option
                key={option.id}
                value={option.id}
                selected={value === option.id}
                onClick={() => onOptionSelect(option.id)}
              >
                <RampLogo/>
                {option.label}
              </Option>
            )}
          </Dropdown>
      }
    </StyledWrapper>
  )
}

SelectBuy.defaultProps = {
  closeOnSelect: true
}

export default SelectBuy
