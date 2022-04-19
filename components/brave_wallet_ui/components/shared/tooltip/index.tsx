import * as React from 'react'

import {
  TipWrapper,
  Tip,
  Pointer,
  ActionNotification,
  TipAndChildrenWrapper
} from './style'

export interface Props {
  children?: React.ReactNode
  position?: 'left' | 'right'
  actionText?: string
  isVisible: boolean
  isAddress?: boolean
  isActionVisible?: boolean
  disableHoverEvents?: boolean
  text: React.ReactNode
  verticalPosition?: 'above' | 'below'
  horizontalMarginPx?: number
  pointerPosition?: 'left' | 'right' | 'center'
}

function Tooltip ({
  actionText,
  children,
  disableHoverEvents,
  horizontalMarginPx,
  isActionVisible,
  isAddress,
  isVisible,
  pointerPosition,
  position,
  text,
  verticalPosition = 'below'
}: Props) {
  const [active, setActive] = React.useState(!!disableHoverEvents)

  const showTip = () => {
    !disableHoverEvents && setActive(true)
  }

  const hideTip = () => {
    !disableHoverEvents && setActive(false)
  }

  const toolTipPointer = React.useMemo(() => (
    <Pointer
      position={pointerPosition ?? position ?? 'center'}
      verticalPosition={verticalPosition ?? 'below'}
    />
  ), [position, verticalPosition, pointerPosition])

  const toolTip = React.useMemo(() => active && isVisible && (
    <TipWrapper
      position={position ?? 'center'}
      verticalPosition={verticalPosition ?? 'below'}
      horizontalMarginPx={horizontalMarginPx}
    >

      {!isActionVisible && verticalPosition === 'below' && toolTipPointer}

      {isActionVisible
        ? <ActionNotification
            verticalPosition={verticalPosition ?? 'below'}
            position={position ?? 'center'}
          >
            {actionText}
          </ActionNotification>
        : <Tip
            isAddress={isAddress}
            position={position ?? 'center'}
            verticalPosition={verticalPosition ?? 'below'}
          >
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
    horizontalMarginPx,
    isActionVisible
  ])

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
