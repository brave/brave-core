import * as React from 'react'
import { getLocale } from '../../../../common/locale'

// Styled Components
import { StyledWrapper, FavIcon, URLText, PanelTitle } from './style'

export interface Props {
  url: string
  hideTitle?: boolean
}

function ConnectHeader (props: Props) {
  const { url, hideTitle } = props
  return (
    <StyledWrapper>
      <FavIcon src={`chrome://favicon/size/64@1x/${url}`} />
      <URLText>{url}</URLText>
      {!hideTitle &&
        <PanelTitle>{getLocale('braveWalletConnectWithSiteHeaderTitle')}</PanelTitle>
      }
    </StyledWrapper>
  )
}

export default ConnectHeader
