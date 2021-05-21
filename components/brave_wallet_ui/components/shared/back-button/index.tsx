import * as React from 'react'
import locale from '../../../constants/locale'
// Styled Components
import {
  StyledWrapper,
  BackIcon
} from './style'

export interface Props {
  onSubmit: () => void
}

const BackButton = (props: Props) => {
  const { onSubmit } = props
  return (
    <StyledWrapper onClick={onSubmit}><BackIcon />{locale.back}</StyledWrapper>
  )
}
export default BackButton
