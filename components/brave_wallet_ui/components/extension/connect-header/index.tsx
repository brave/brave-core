import * as React from 'react'
import locale from '../../../constants/locale'

// Styled Components
import { StyledWrapper, FavIcon, URLText, PanelTitle } from './style'

export interface Props {
  url: string
}

function ConnectHeader (props: Props) {
  const { url } = props
  return (
    <StyledWrapper>
      <FavIcon src={`chrome://favicon/size/64@1x/${url}`} />
      <URLText>{url}</URLText>
      <PanelTitle>{locale.connectWithSiteHeaderTitle}</PanelTitle>
    </StyledWrapper>
  )
}

export default ConnectHeader
