// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

// Queries
import {
  useGetRewardsInfoQuery, //
} from '../../../common/slices/api.slice'
import { emptyRewardsInfo } from '../../../common/async/base-query-cache'

// Utils
import { getLocale } from '../../../../common/locale'

// Styled Components
import { ButtonMenu } from './wellet-menus.style'

const onClickRewardsSettings = () => {
  chrome.tabs.create(
    {
      url: 'brave://rewards',
    },
    () => {
      if (chrome.runtime.lastError) {
        console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
      }
    },
  )
}

export const RewardsMenu = () => {
  // Queries
  const {
    data: { accountLink: providerAccountUrl, providerName } = emptyRewardsInfo,
  } = useGetRewardsInfoQuery()

  // Computed
  const providerButtonText = getLocale('braveWalletViewOn').replace(
    '$1',
    providerName,
  )

  // Methods
  const onClickOnProviderAccount = () => {
    if (!providerAccountUrl) {
      return
    }
    chrome.tabs.create(
      {
        url: providerAccountUrl,
      },
      () => {
        if (chrome.runtime.lastError) {
          console.error(
            'tabs.create failed: ' + chrome.runtime.lastError.message,
          )
        }
      },
    )
  }

  return (
    <ButtonMenu placement='bottom-end'>
      <Button
        fab
        slot='anchor-content'
        kind='plain-faint'
        size='large'
      >
        <Icon name='more-vertical' />
      </Button>
      <leo-menu-item onClick={onClickOnProviderAccount}>
        <Icon name='launch' />
        {providerButtonText}
      </leo-menu-item>
      <leo-menu-item onClick={onClickRewardsSettings}>
        <Icon name='product-bat-outline' />
        {getLocale('braveWalletRewardsSettings')}
      </leo-menu-item>
    </ButtonMenu>
  )
}
