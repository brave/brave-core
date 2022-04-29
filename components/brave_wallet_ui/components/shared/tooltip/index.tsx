import * as React from 'react'

import {
  StyledWrapper,
  Tip,
  Pointer
} from './style'

export interface Props {
  children?: React.ReactNode
  position?: 'left' | 'right'
  text: string
  isVisible: boolean
  isAddress?: boolean
}

function Tooltip (props: Props) {
  const { children, text, position, isVisible, isAddress } = props
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
      {active && isVisible && (
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
    </StyledWrapper>
  )
}

Tooltip.defaultProps = {
  isVisible: true
}

export default Tooltip
