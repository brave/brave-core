// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
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
