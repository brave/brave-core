import * as React from 'react'
import { getLocale } from '../../../../common/locale'
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
    <StyledWrapper onClick={onSubmit}>
      <BackIcon />
      {getLocale('braveWalletBack')}
    </StyledWrapper>
  )
}
export default BackButton
