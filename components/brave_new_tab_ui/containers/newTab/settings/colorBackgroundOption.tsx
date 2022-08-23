// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

import {
  StyledCustomBackgroundOption,
  StyledCustomBackgroundOptionColor,
  StyledSelectionBorder
} from '../../../components/default'

interface Props {
  color: string
  selected: boolean
  onSelectValue: (color: string) => void
}

export default function SolidColorBackgroundOption ({ color, selected, onSelectValue }: Props) {
  return (
    <StyledCustomBackgroundOption onClick={_ => onSelectValue(color)}>
      <StyledSelectionBorder selected={selected}>
          <StyledCustomBackgroundOptionColor
            colorValue={color}
            selected={selected}/>
      </StyledSelectionBorder>
    </StyledCustomBackgroundOption>
  )
}
