// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Images
import BraveIcon from '../../../../assets/svg-icons/brave-icon.svg'

// Components
import { SiteOrigin } from '../../../shared/create-site-origin'

// Styled Components
import { URLText } from '../../shared-panel-styles'
import { FavIcon } from './style'

// Types
import { BraveWallet } from '../../../../constants/types'

// Utils
import {
  getIsBraveWalletOrigin,
  isComponentInStorybook,
} from '../../../../utils/string-utils'

interface Props {
  originInfo: BraveWallet.OriginInfo
}

const getFaviconSrc = (originInfo: BraveWallet.OriginInfo) => {
  if (getIsBraveWalletOrigin(originInfo)) {
    return BraveIcon
  }
  if (isComponentInStorybook()) {
    return `${originInfo.originSpec}/favicon.png`
  }
  return `chrome://favicon2?size=64&pageUrl=${encodeURIComponent(originInfo.originSpec)}`
}

export function Origin(props: Props) {
  const { originInfo } = props
  return (
    <>
      <FavIcon src={getFaviconSrc(originInfo)} />
      <URLText
        textColor='secondary'
        variant='xSmall.regular'
      >
        <SiteOrigin
          originSpec={originInfo.originSpec}
          eTldPlusOne={originInfo.eTldPlusOne}
        />
      </URLText>
    </>
  )
}
