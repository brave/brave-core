// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Styled Components
import {
  HiddenCheckBox,
  Label,
  StyledCheckbox,
  StyledIcon
} from './standard-checkbox.style'
import { Row } from '../../shared-swap.styles'

interface Props {
  label: string
  id: string
  isChecked: boolean
  labelSize?: '12px' | '14px'
  isBold?: boolean
  onChange: (id: string, checked: boolean) => void
}

export const StandardCheckbox = (props: Props) => {
  const { label, id, onChange, isChecked, labelSize, isBold } = props

  // Methods
  const handleChange = (event: React.ChangeEvent<HTMLInputElement>) => {
    onChange(event.target.id, event.target.checked)
  }

  return (
    <Row>
      <HiddenCheckBox
        type='checkbox'
        name='checkbox'
        id={id}
        onChange={handleChange}
        checked={isChecked}
      />
      <Label
        isChecked={isChecked}
        htmlFor={id}
        labelSize={labelSize}
        isBold={isBold}
      >
        <StyledCheckbox isChecked={isChecked}>
          {isChecked && (
            <StyledIcon
              size={16}
              name='check-normal'
            />
          )}
        </StyledCheckbox>
        {label}
      </Label>
    </Row>
  )
}
