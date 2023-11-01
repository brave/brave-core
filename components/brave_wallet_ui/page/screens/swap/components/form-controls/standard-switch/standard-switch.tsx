// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Styled Components
import { Input, Label, Switch } from './standard-switch.style'

interface Props {
  isChecked: boolean
  onSetIsChecked: (isChecked: boolean) => void
}

export const StandardSwitch = (props: Props) => {
  const { isChecked, onSetIsChecked } = props

  const handleChange = (event: React.ChangeEvent<HTMLInputElement>) =>
    onSetIsChecked(event.target.checked)

  return (
    <Label>
      <Input
        checked={isChecked}
        type='checkbox'
        onChange={handleChange}
      />
      <Switch />
    </Label>
  )
}
