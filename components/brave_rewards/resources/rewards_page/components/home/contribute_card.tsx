/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import { useAppState } from '../../lib/app_model_context'
import { useLocaleContext } from '../../lib/locale_strings'
import { NewTabLink } from '../../../shared/components/new_tab_link'
import { getCreatorIconSrc, getCreatorPlatformIcon } from '../../lib/creator_icon'

import { style } from './contribute_card.style'

export function ContributeCard() {
  const { getString } = useLocaleContext()
  const [creator, externalWallet] = useAppState((state) => [
    state.currentCreator,
    state.externalWallet
  ])

  if (!creator) {
    return null
  }

  const { site, banner } = creator

  function renderOrigin() {
    if (site.platform) {
      return <Icon name={getCreatorPlatformIcon(site)} forceColor />
    }

    return <span>{site.id}</span>
  }

  return (
    <div className='content-card' {...style}>
      <section>
        <NewTabLink href={site.url} className='icon'>
          <img src={getCreatorIconSrc(site)} alt='Site icon' />
        </NewTabLink>
        <NewTabLink href={site.url} className='text'>
          <span className='name'>
            <span>{banner.title || site.name}</span>
            <Icon name='verification-filled-color' />
          </span>
          <span className='origin'>
            {renderOrigin()}
          </span>
        </NewTabLink>
        <Button kind={externalWallet ? 'filled' : 'outline'}>
          {getString('contributeButtonLabel')}
        </Button>
      </section>
    </div>
  )
}
