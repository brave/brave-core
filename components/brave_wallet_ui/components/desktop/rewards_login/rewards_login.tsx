// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'

// Types
import {
  RewardsExternalWallet
} from '../../../common/async/brave_rewards_api_proxy'

// Utils
import { getLocale } from '../../../../common/locale'
import {
  getRewardsProviderName
} from '../../../utils/rewards_utils'

// Styled Components
import {
  LoginWrapper,
  InfoIcon,
  InfoText,
  ButtonWrapper
} from './rewards_login.style'
import { Row } from '../../shared/style'

interface Props {
  externalRewardsInfo: RewardsExternalWallet | null | undefined
}

export const RewardsLogin = (props: Props) => {
  const { externalRewardsInfo } = props

  // Computed
  const provider = externalRewardsInfo?.provider ?? ''
  const providerReconnectUrl = externalRewardsInfo?.links.reconnect
  const providerName = getRewardsProviderName(provider)
  const loginDescription =
    getLocale('braveWalletBraveRewardsLoggedOutDescription')
      .replace('$1', providerName)

  // Methods
  const onClickLogin = () => {
    if (!providerReconnectUrl) {
      return
    }
    chrome.tabs.create(
      {
        url: providerReconnectUrl
      }, () => {
        if (chrome.runtime.lastError) {
          console.error(
            'tabs.create failed: '
            + chrome.runtime.lastError.message
          )
        }
      })
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
