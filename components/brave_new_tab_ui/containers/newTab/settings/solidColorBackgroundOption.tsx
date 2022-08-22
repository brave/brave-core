// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

import {
  StyledCustomBackgroundOption,
  StyledCustomBackgroundOptionSolidColor,
  StyledSelectionBorder
} from '../../../components/default'

interface Props {
  color: string
  selected: boolean
  useSolidColorBackground: (color: string) => void
}

function SolidColorBackgroundOption (props: Props) {
  const { color, selected, useSolidColorBackground } = props

  return (
    <StyledCustomBackgroundOption onClick={_ => useSolidColorBackground(color)}>
      <StyledSelectionBorder selected={selected}>
          <StyledCustomBackgroundOptionSolidColor
            color={color}
            selected={selected}/>
      </StyledSelectionBorder>
    </StyledCustomBackgroundOption>
  )
}

export default SolidColorBackgroundOption
