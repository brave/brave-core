import * as React from 'react'
import { getLocale } from '../../../../common/locale'

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
      <PanelTitle>{getLocale('braveWalletConnectWithSiteHeaderTitle')}</PanelTitle>
    </StyledWrapper>
  )
}

export default ConnectHeader
