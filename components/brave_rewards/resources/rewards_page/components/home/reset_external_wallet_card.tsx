/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import { useLocaleContext } from '../../lib/locale_strings'
import { useAppState } from '../../lib/app_model_context'
import formatMessage from '$web-common/formatMessage'
import { getExternalWalletProviderName } from '../../../shared/lib/external_wallet'
import { NewTabLink } from '../../../shared/components/new_tab_link'
import { useSwitchAccountRouter } from '../../lib/connect_account_router'

import * as urls from '../../../shared/lib/rewards_urls'

import { style } from './reset_external_wallet_card.style'

export function ResetExternalWalletCard() {
  const { getString } = useLocaleContext()
  const openSwitchAccount = useSwitchAccountRouter()
  const externalWallet = useAppState((s) => s.externalWallet)

  if (!externalWallet) {
    return null
  }

  const providerName = getExternalWalletProviderName(externalWallet.provider)

  function learnMoreURL() {
    switch (externalWallet?.provider) {
      case 'gemini':
        return urls.geminiDeprecationLearnMoreURL
      default:
        return ''
    }
  }

  function renderLearnMore(content: any) {
    const url = learnMoreURL()
    if (!url) {
      return null
    }
    return (
      <NewTabLink
        key='learn-more'
        href={url}
      >
        {content}
      </NewTabLink>
    )
  }

  return (
    <div data-css-scope={style.scope}>
      <div className='icon'>
        <Icon name='warning-triangle-filled' />
      </div>
      <h3>
        {formatMessage(getString('resetExternalWalletTitle'), [providerName])}
      </h3>
      <p>
        {formatMessage(getString('resetExternalWalletText'), {
          placeholders: { $1: providerName },
          tags: { $2: renderLearnMore },
        })}
      </p>
      <Button onClick={openSwitchAccount}>
        {getString('resetExternalWalletButtonLabel')}
      </Button>
      <p className='note'>
        {formatMessage(getString('resetExternalWalletNote'), [providerName])}
      </p>
    </div>
  )
}
