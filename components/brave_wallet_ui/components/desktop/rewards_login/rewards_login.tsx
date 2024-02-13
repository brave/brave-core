// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'

// Types
import {
  ExternalWalletProvider //
} from '../../../../brave_rewards/resources/shared/lib/external_wallet'

// Utils
import { getLocale } from '../../../../common/locale'
import { getRewardsProviderName } from '../../../utils/rewards_utils'
import {
  reconnectURL //
} from '../../../../brave_rewards/resources/shared/lib/rewards_urls'

// Styled Components
import {
  LoginWrapper,
  InfoIcon,
  InfoText,
  ButtonWrapper
} from './rewards_login.style'
import { Row } from '../../shared/style'

interface Props {
  provider: ExternalWalletProvider | undefined
}

export const RewardsLogin = ({ provider }: Props) => {
  // Computed
  const providerName = getRewardsProviderName(provider)
  const loginDescription = getLocale(
    'braveWalletBraveRewardsLoggedOutDescription'
  ).replace('$1', providerName)

  // Methods
  const onClickLogin = () => {
    chrome.tabs.create(
      {
        url: reconnectURL
      },
      () => {
        if (chrome.runtime.lastError) {
          console.error(
            'tabs.create failed: ' + chrome.runtime.lastError.message
          )
        }
      }
    )
  }

  return (
    <LoginWrapper>
      <Row
        width='unset'
        margin='0px 8px 0px 0px'
      >
        <InfoIcon />
        <InfoText
          textSize='14px'
          textAlign='left'
        >
          {loginDescription}
        </InfoText>
      </Row>
      <ButtonWrapper>
        <Button
          onClick={onClickLogin}
          kind='plain'
          size='tiny'
        >
          {getLocale('braveWalletLogIn')}
        </Button>
      </ButtonWrapper>
    </LoginWrapper>
  )
}
