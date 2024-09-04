/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import { useAppState } from '../../lib/app_model_context'
import { useLocaleContext } from '../../lib/locale_strings'
import { EventHubContext } from '../../lib/event_hub'
import { isSelfCustodyProvider } from '../../../shared/lib/external_wallet'
import { TabOpenerContext } from '../../../shared/components/new_tab_link'
import { getCreatorIconSrc, getCreatorPlatformIcon } from '../../lib/creator_icon'

import { style } from './contribute_card.style'

export function ContributeCard() {
  const { getString } = useLocaleContext()
  const tabOpener = React.useContext(TabOpenerContext)
  const eventHub = React.useContext(EventHubContext)
  const [creator, externalWallet] = useAppState((state) => [
    state.currentCreator,
    state.externalWallet
  ])

  if (!creator) {
    return null
  }

  const { site, banner } = creator

  function getName() {
    return banner.title || site.name
  }

  function renderOrigin() {
    if (site.platform) {
      return <>
        <span>{site.name}</span>
        <Icon name={getCreatorPlatformIcon(site)} forceColor />
      </>
    }
    return <span>{site.id}</span>
  }

  function hasMatchingCustodialProvider() {
    return Boolean(
      creator && externalWallet &&
      creator.supportedWalletProviders.includes(externalWallet.provider) &&
      !isSelfCustodyProvider(externalWallet.provider)
    )
  }

  function onContributeClick() {
    if (!hasMatchingCustodialProvider() && creator?.banner.web3URL) {
      tabOpener.openTab(creator.banner.web3URL)
    } else {
      eventHub.dispatch('open-modal', 'contribute')
    }
  }

  return (
    <div className='content-card' {...style}>
      <section>
        <div className='icon'>
          <img src={getCreatorIconSrc(site)} alt='Site icon' />
        </div>
        <div className='text'>
          <span className='name'>
            <span>{getName()}</span>
            <Icon name='verification-filled-color' />
          </span>
          <span className='origin'>
            {renderOrigin()}
          </span>
        </div>
        <Button
          kind={externalWallet ? 'filled' : 'outline'}
          onClick={onContributeClick}
        >
          {getString('contributeButtonLabel')}
        </Button>
      </section>
    </div>
  )
}
