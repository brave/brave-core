import * as React from 'react'
import * as S from './style'
import { CaratStrongRightIcon } from 'brave-ui/components/icons'

interface Props {
  region: string
  onClick: Function
}

function RegionSelectorButton (props: Props) {
  const handleOnClick = () => {
    props.onClick()
  }

  return (
    <S.Box
      type='button'
      onClick={handleOnClick}
    >
      <S.RegionLabel>{props.region}</S.RegionLabel>
      <CaratStrongRightIcon />
    </S.Box>
  )
}

export default RegionSelectorButton
