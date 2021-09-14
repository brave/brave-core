import * as React from 'react'

import {
  StyledWrapper,
  Tip,
  Pointer
} from './style'

export interface Props {
  children?: React.ReactNode
  text: string
  isDisabled: boolean
  position: 'left' | 'right'
}

function PanelTooltip (props: Props) {
  const { children, text, isDisabled, position } = props
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
          <Tip position={position}>
            {text}
          </Tip>
        </>
      )}
    </StyledWrapper>
  )
}

export default PanelTooltip
