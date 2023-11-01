// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Styled Components
import { Label, Radio, Wrapper } from './standard-radio.style'

interface Props {
  label: string
  id: string
  isChecked: boolean
  onSetIsChecked: (value: string) => void
}

export const StandardRadio = (props: Props) => {
  const { label, id, onSetIsChecked, isChecked } = props

  const handleChange = (event: React.ChangeEvent<HTMLInputElement>) => {
    onSetIsChecked(event.target.value)
  }

  return (
    <Wrapper>
      <Radio
        type='radio'
        name='radio'
        id={id}
        value={id}
        onChange={(event) => handleChange(event)}
        checked={isChecked}
      />
      <Label
        isChecked={isChecked}
        htmlFor={id}
      >
        {label}
      </Label>
    </Wrapper>
  )
}
