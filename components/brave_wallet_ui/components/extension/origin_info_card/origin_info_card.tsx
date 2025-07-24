// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { BraveWallet } from '../../../constants/types'

// Components
import { CreateSiteOrigin } from '../../shared/create-site-origin'

// Styled Components
import {
  FavIcon,
  OriginName,
  OriginUrl,
  StyledWrapper,
} from './origin_info_card.style'
import { Column } from '../../shared/style'

interface Props {
  origin: BraveWallet.OriginInfo
}

export const OriginInfoCard = (props: Props) => {
  const { origin } = props

  return (
    <StyledWrapper
      padding='10px 16px'
      gap='16px'
      justifyContent='flex-start'
    >
      <FavIcon
        src={
          'chrome://favicon2?size=64&pageUrl='
          + encodeURIComponent(origin.originSpec)
        }
      />
      <Column alignItems='flex-start'>
        <OriginName textColor='primary'>{origin.eTldPlusOne}</OriginName>
        <Column
          gap='4px'
          alignItems='flex-start'
        >
          <OriginUrl textColor='tertiary'>
            <CreateSiteOrigin
              originSpec={origin.originSpec}
              eTldPlusOne={origin.eTldPlusOne}
            />
          </OriginUrl>
        </Column>
      </Column>
    </StyledWrapper>
  )
}
