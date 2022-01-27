import * as React from 'react'

import { StyledWrapper, Star } from './style'

export interface Props {
  active: boolean
  onCheck?: (status: boolean) => void
}

const AssetWishlistStar = (props: Props) => {
  const { active, onCheck } = props

  const onClick = () => {
    if (onCheck) {
      onCheck(!active)
    }
  }
  return (
    <StyledWrapper>
      <Star
      active={active}
      onClick={onClick}
    />
    </StyledWrapper>
  )
}

export default AssetWishlistStar
