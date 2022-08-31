// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// style
import {
  StyledWrapper,
  Tip,
  Pointer
} from './style'

export interface Props {
  children?: React.ReactNode
  text: string
  isDisabled: boolean
  maxTextWidth?: React.CSSProperties['maxWidth']
}

function BuySendSwapTooltip (props: Props) {
  const { children, text, isDisabled, maxTextWidth } = props
  const [active, setActive] = React.useState(false)

  const showTip = () => {
    setActive(true)
  }

  const hideTip = () => {
    setActive(false)
  }

  return (
    <StyledWrapper
      onMouseEnter={showTip}
      onMouseLeave={hideTip}
    >
      {children}
      {active && isDisabled && (
        <>
          <Pointer />
          <Tip maxWidth={maxTextWidth}>
            {text}
          </Tip>
        </>
      )}
    </StyledWrapper>
  )
}

export default BuySendSwapTooltip
