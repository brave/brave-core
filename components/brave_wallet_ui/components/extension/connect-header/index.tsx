import * as React from 'react'
import { OriginInfo } from '../../../constants/types'

// Utils
import { getLocale } from '../../../../common/locale'

// Components
import { CreateSiteOrigin } from '../../shared'

// Styled Components
import {
  StyledWrapper,
  FavIcon,
  PanelTitle
} from './style'

import { URLText } from '../shared-panel-styles'

export interface Props {
  originInfo: OriginInfo
  hideTitle?: boolean
}

function ConnectHeader (props: Props) {
  const { originInfo, hideTitle } = props

  return (
    <StyledWrapper>
      <FavIcon src={`chrome://favicon/size/64@1x/${originInfo.origin}`} />
      <URLText>
        <CreateSiteOrigin originInfo={originInfo} />
      </URLText>
      {!hideTitle &&
        <PanelTitle>{getLocale('braveWalletConnectWithSiteHeaderTitle')}</PanelTitle>
      }
    </StyledWrapper>
  )
}

export default ConnectHeader
