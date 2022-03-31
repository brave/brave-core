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
}

function BuySendSwapTooltip (props: Props) {
  const { children, text, isDisabled } = props
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
          <Tip>
            {text}
          </Tip>
        </>
      )}
    </StyledWrapper>
  )
}

export default BuySendSwapTooltip
