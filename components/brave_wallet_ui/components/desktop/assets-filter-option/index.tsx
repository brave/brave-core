// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { Option } from './style'

export interface Props {
  label: string
  value: string
  selected: boolean
  onSelect: (value: string) => void
}

export const AssetsFilterOption = (props: Props) => {
  const { selected, label, value, onSelect } = props

  const onClick = () => {
    onSelect(value)
  }

  return (
    <Option
      selected={selected}
      onClick={onClick}
    >
    {label}
    </Option>
  )
}
