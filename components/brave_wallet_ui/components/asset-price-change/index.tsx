import * as React from 'react'

import { StyledWrapper, PriceChange, ArrowDown, ArrowUp } from './style'

export interface Props {
  change: number
}

const AssetPriceChange = (props: Props) => {
  const { change } = props

  const absChange = Math.abs(change)

  return (
    <StyledWrapper change={change}>
      {change > 0 ? <ArrowUp /> : <ArrowDown />}
      <PriceChange change={change}>
        {absChange}%
      </PriceChange>
    </StyledWrapper>
  )
}

export default AssetPriceChange
