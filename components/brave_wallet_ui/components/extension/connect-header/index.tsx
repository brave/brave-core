import * as React from 'react'

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
import { BraveWallet } from '../../../constants/types'

export interface Props {
  originInfo: BraveWallet.OriginInfo
  hideTitle?: boolean
}

function ConnectHeader (props: Props) {
  const { originInfo, hideTitle } = props

  return (
    <StyledWrapper>
      <FavIcon src={`chrome://favicon/size/64@1x/${originInfo.originSpec}`} />
      <URLText>
        <CreateSiteOrigin
          originSpec={originInfo.originSpec}
          eTldPlusOne={originInfo.eTldPlusOne}
        />
      </URLText>
      {!hideTitle &&
        <PanelTitle>{getLocale('braveWalletConnectWithSiteHeaderTitle')}</PanelTitle>
      }
    </StyledWrapper>
  )
}

export default ConnectHeader
