// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Images
import BraveIcon from '../../../assets/svg-icons/brave-icon.svg'

// Types
import { BraveWallet } from '../../../constants/types'

// Utils
import {
  getIsBraveWalletOrigin,
  isComponentInStorybook,
} from '../../../utils/string-utils'

// Hooks
import { useIsDAppVerified } from '../../../common/hooks/use_is_dapp_verified'

// Components
import { CreateSiteOrigin } from '../../shared/create-site-origin'
import { VerifiedLabel } from '../../shared/verified_label/verified_label'

// Styled Components
import {
  FavIcon,
  OriginName,
  OriginUrl,
  StyledWrapper,
} from './origin_info_card.style'
import { Column } from '../../shared/style'

const isStorybook = isComponentInStorybook()

interface Props {
  origin: BraveWallet.OriginInfo
  noBackground?: boolean
  provider?: string
  orientation?: 'horizontal' | 'vertical'
}

export const OriginInfoCard = (props: Props) => {
  const {
    origin,
    noBackground = false,
    provider,
    orientation = 'horizontal',
  } = props

  // Hooks
  const { isDAppVerified, dapp } = useIsDAppVerified(origin)

  // Computed
  const isBraveWallet = getIsBraveWalletOrigin(origin)

  const dappIcon = dapp
    ? isStorybook
      ? dapp.logo
      : `chrome://image?url=${encodeURIComponent(dapp.logo)}&staticEncode=true`
    : undefined

  const iconSrc =
    dappIcon
    ?? 'chrome://favicon2?size=64&pageUrl='
      + encodeURIComponent(origin.originSpec)

  return (
    <StyledWrapper
      padding='10px 16px'
      gap={orientation === 'vertical' ? '8px' : '16px'}
      justifyContent={orientation === 'vertical' ? 'center' : 'flex-start'}
      noBackground={noBackground}
      orientation={orientation}
      data-testid='origin-info-card'
    >
      <FavIcon
        src={isBraveWallet ? BraveIcon : iconSrc}
        bigger={orientation === 'vertical'}
      />
      <Column alignItems={orientation === 'vertical' ? 'center' : 'flex-start'}>
        {isBraveWallet && provider && (
          <OriginName
            textColor='primary'
            textAlign={orientation === 'horizontal' ? 'left' : undefined}
          >
            {provider}
          </OriginName>
        )}
        <OriginName
          textColor='primary'
          textAlign={orientation === 'horizontal' ? 'left' : undefined}
        >
          {dapp ? dapp.name : origin.eTldPlusOne}
        </OriginName>
        <Column
          gap='4px'
          alignItems={orientation === 'vertical' ? 'center' : 'flex-start'}
        >
          <OriginUrl
            textColor='tertiary'
            textAlign={orientation === 'horizontal' ? 'left' : undefined}
          >
            <CreateSiteOrigin
              originSpec={origin.originSpec}
              eTldPlusOne={origin.eTldPlusOne}
            />
          </OriginUrl>
          {(isDAppVerified || isBraveWallet) && <VerifiedLabel />}
        </Column>
      </Column>
    </StyledWrapper>
  )
}
