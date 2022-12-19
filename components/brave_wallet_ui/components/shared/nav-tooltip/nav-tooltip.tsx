// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Styled Components
import { Pointer, Tip } from './nav-tooltip.style'

export interface Props {
  text: string
  horizontalAlign?: 'left' | 'right' | 'center'
  orientation: 'top' | 'bottom' | 'right'
  distance: number
  showTip: boolean
  isSwap?: boolean
}

export const NavTooltip = (props: Props) => {
  const { text, horizontalAlign, orientation, distance, showTip, isSwap } = props

  if (!showTip) {
    return null
  }

  return (
    <>
      <Pointer distance={distance} orientation={orientation} isSwap={isSwap} />
      <Tip
        distance={distance}
        orientation={orientation}
        horizontalAlign={horizontalAlign}
        isSwap={isSwap}
      >
        {text}
      </Tip>
    </>
  )
}

export default NavTooltip
