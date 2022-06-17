import * as React from 'react'

import {
  StyledWrapper,
  Tip,
  Pointer,
  ActionNotification
} from './style'

export interface Props {
  children?: React.ReactNode
  position?: 'left' | 'right'
  text: string
  actionText?: string
  isVisible: boolean
  isAddress?: boolean
  isActionVisible?: boolean
}

function Tooltip (props: Props) {
  const { children, actionText, text, position, isVisible, isAddress, isActionVisible } = props
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
      {active && isVisible && !isActionVisible && (
        <>
          <Pointer position={position ?? 'center'} />
          <Tip
            isAddress={isAddress}
            position={position ?? 'center'}
          >
            {text}
          </Tip>
        </>
      )}
      {isActionVisible && <ActionNotification position='center'>{actionText}</ActionNotification>}
    </StyledWrapper>
  )
}

Tooltip.defaultProps = {
  isVisible: true
}

export default Tooltip
