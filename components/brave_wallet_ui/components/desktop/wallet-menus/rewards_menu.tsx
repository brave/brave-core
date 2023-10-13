// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Queries
import {
  useGetExternalRewardsWalletQuery
} from '../../../common/slices/api.slice'

// Utils
import { getLocale } from '../../../../common/locale'
import {
  getRewardsProviderName
} from '../../../utils/rewards_utils'

// Styled Components
import {
  StyledWrapper,
  PopupButton,
  PopupButtonText,
  ButtonIcon
} from './wellet-menus.style'

const onClickRewardsSettings = () => {
  chrome.tabs.create(
    {
      url: 'brave://rewards'
    }, () => {
      if (chrome.runtime.lastError) {
        console.error(
          'tabs.create failed: '
          + chrome.runtime.lastError.message
        )
      }
    })
}

export const RewardsMenu = () => {

  // Queries
  const { data: externalRewardsInfo } = useGetExternalRewardsWalletQuery()

  // Computed
  const provider = externalRewardsInfo?.provider ?? ''
  const providerAccountUrl = externalRewardsInfo?.links.account
  const providerButtonText =
    getLocale('braveWalletViewOn')
      .replace('$1', getRewardsProviderName(provider))

  // Methods
  const onClickOnProviderAccount = () => {
    if (!providerAccountUrl) {
      return
    }
    chrome.tabs.create(
      {
        url: providerAccountUrl
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
    <StyledWrapper yPosition={26}>
      <PopupButton onClick={onClickOnProviderAccount}>
        <ButtonIcon name='launch' />
        <PopupButtonText>
          {providerButtonText}
        </PopupButtonText>
      </PopupButton>
      <PopupButton onClick={onClickRewardsSettings}>
        <ButtonIcon name='product-bat-outline' />
        <PopupButtonText>
          {getLocale('braveWalletRewardsSettings')}
        </PopupButtonText>
      </PopupButton>
    </StyledWrapper>
  )
}
