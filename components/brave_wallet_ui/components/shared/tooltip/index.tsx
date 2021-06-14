import * as React from 'react'

import {
  StyledWrapper,
  Tip,
  Pointer
} from './style'

export interface Props {
  children?: React.ReactNode
  text: string
}

function Tooltip (props: Props) {
  const { children, text } = props
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
      {active && (
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

export default Tooltip
