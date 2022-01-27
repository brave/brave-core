import * as React from 'react'

import { StyledWrapper, PriceChange, ArrowDown, ArrowUp } from './style'

export interface Props {
  isDown: boolean
  priceChangePercentage: string
}

const AssetPriceChange = (props: Props) => {
  const { isDown, priceChangePercentage } = props

  return (
    <StyledWrapper isDown={isDown}>
      {isDown ? <ArrowDown /> : <ArrowUp /> }
      <PriceChange>
        {priceChangePercentage}
      </PriceChange>
    </StyledWrapper>
  )
}

export default AssetPriceChange
