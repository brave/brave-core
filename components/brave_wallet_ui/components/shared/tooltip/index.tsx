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
  isVisible?: boolean
  isAddress?: boolean
  isActionVisible?: boolean
  disableHoverEvents?: boolean
  text: React.ReactNode
  verticalPosition?: 'above' | 'below'
  horizontalMargin?: string
  pointerPosition?: 'left' | 'right' | 'center'
}

export const Tooltip: React.FC<Props> = ({
  actionText,
  children,
  disableHoverEvents,
  horizontalMargin,
  isActionVisible,
  isAddress,
  isVisible = true,
  pointerPosition,
  position,
  text,
  verticalPosition = 'below'
}) => {
  const [active, setActive] = React.useState(!!disableHoverEvents)

  const showTip = () => {
    !disableHoverEvents && setActive(true)
  }

  const hideTip = () => {
    !disableHoverEvents && setActive(false)
  }

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
      horizontalMargin={horizontalMargin}
    >

      {!isActionVisible && verticalPosition === 'below' && toolTipPointer}

      {isActionVisible
        ? <ActionNotification>
            {actionText}
          </ActionNotification>

        : <Tip isAddress={isAddress}>
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
    isActionVisible,
    horizontalMargin
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
