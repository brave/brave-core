// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// style
import {
  TipWrapper,
  Tip,
  Pointer,
  ActionNotification,
  TipAndChildrenWrapper
} from './style'

export interface ToolTipProps {
  children?: React.ReactNode
  position?: 'left' | 'right'
  actionText?: string
  isVisible?: boolean
  isAddress?: boolean
  isActionVisible?: boolean
  disableHoverEvents?: boolean
  text: React.ReactNode
  verticalPosition?: 'above' | 'below'
  pointerPosition?: 'left' | 'right' | 'center'
  minWidth?: React.CSSProperties['minWidth']
  maxWidth?: React.CSSProperties['maxWidth']
}

export const Tooltip: React.FC<ToolTipProps> = ({
  actionText,
  children,
  disableHoverEvents,
  isActionVisible,
  isAddress,
  isVisible = true,
  pointerPosition,
  position,
  text,
  verticalPosition = 'below',
  maxWidth,
  minWidth
}) => {
  // state
  const [active, setActive] = React.useState(!!disableHoverEvents)

  // methods
  const showTip = React.useCallback(() => {
    !disableHoverEvents && setActive(true)
  }, [disableHoverEvents])

  const hideTip = React.useCallback(() => {
    !disableHoverEvents && setActive(false)
  }, [disableHoverEvents])

  // memos
  const toolTipPointer = React.useMemo(() => (
    <Pointer
      position={pointerPosition ?? 'center'}
      verticalPosition={verticalPosition ?? 'below'}
    />
  ), [position, verticalPosition, pointerPosition])

  const toolTip = React.useMemo(() => active && isVisible && (
    <TipWrapper
      position={position ?? 'center'}
      verticalPosition={verticalPosition ?? 'below'}
    >

      {!isActionVisible && verticalPosition === 'below' && toolTipPointer}

      {isActionVisible
        ? <ActionNotification>
            {actionText}
          </ActionNotification>

        : <Tip maxWidth={maxWidth} minWidth={minWidth} isAddress={isAddress}>
            {text}
          </Tip>
      }
      {!isActionVisible && verticalPosition === 'above' && toolTipPointer}
    </TipWrapper>
  ), [
    active,
    isVisible,
    position,
    verticalPosition,
    isAddress,
    text,
    isActionVisible
  ])

  // render
  return (
    <TipAndChildrenWrapper
      onMouseEnter={showTip}
      onMouseLeave={hideTip}
    >
      {verticalPosition === 'above' && toolTip}
      {children}
      {verticalPosition === 'below' && toolTip}
    </TipAndChildrenWrapper>
  )
}

Tooltip.defaultProps = {
  isVisible: true
}

export default Tooltip
