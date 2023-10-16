/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'

import { LocaleContext } from '../../shared/lib/locale_context'
import { NewTabLink } from '../../shared/components/new_tab_link'
import { GeminiIcon } from '../../shared/components/icons/gemini_icon'
import { UpholdIcon } from '../../shared/components/icons/uphold_icon'
import { BitflyerIcon } from '../../shared/components/icons/bitflyer_icon'
import { ZebPayIcon } from '../../shared/components/icons/zebpay_icon'
import { BraveLogoText } from './icons/brave_logo_text'

import * as urls from '../../shared/lib/rewards_urls'
import * as style from './connect_wallet_modal.style'

function renderProviderIcon (provider: string) {
  switch (provider) {
    case 'bitflyer': return <BitflyerIcon />
    case 'gemini': return <GeminiIcon />
    case 'uphold': return <UpholdIcon />
    case 'zebpay': return <ZebPayIcon />
    default: return null
  }
}

interface ExternalWalletProvider {
  type: string
  name: string
  enabled: boolean
}

interface Props {
  currentCountryCode: string
  providers: ExternalWalletProvider[]
  onContinue: (provider: string) => void
  onClose: () => void
}

export function ConnectWalletModal (props: Props) {
  const { getString } = React.useContext(LocaleContext)

  if (props.providers.length === 0) {
    return null
  }

  function renderProviderButton (provider: ExternalWalletProvider) {
    const onClick = () => {
      if (provider.enabled) {
        props.onContinue(provider.type)
      }
    }

    return (
      <button
        data-test-id='connect-provider-button'
        key={provider.type}
        onClick={onClick}
        disabled={!provider.enabled}
      >
        <style.providerButtonIcon>
          {renderProviderIcon(provider.type)}
        </style.providerButtonIcon>
        <style.providerButtonName>
          {provider.name}
          {
            !provider.enabled &&
              <style.providerButtonMessage>
                {getString('connectWalletProviderNotAvailable')}
              </style.providerButtonMessage>
          }
        </style.providerButtonName>
        {
          provider.enabled &&
            <style.providerButtonCaret>
              <Icon name='carat-right' />
            </style.providerButtonCaret>
        }
      </button>
    )
  }

  return (
    <style.root>
      <style.branding>
        <span className='logo'>
          <Icon name='brave-icon-release-color' />
        </span>
        <span className='logo-text'>
          <BraveLogoText />
        </span>
      </style.branding>
      <style.content>
        <style.nav>
          <Button
            kind='outline'
            size='small'
            fab={true}
            onClick={props.onClose}
          >
            <Icon name='arrow-left' />
          </Button>
        </style.nav>
        <style.title>
          {getString('connectWalletHeader')}
        </style.title>
        <style.text>
          {getString('connectWalletText')}
        </style.text>
        <style.providerSelection>
          <style.providerGroupHeader>
            {getString('connectWalletCustodialHeader')}
            <style.providerGroupHeaderIcon>
              <Icon name='info-filled' />
              <div className='tooltip'>
                <style.providerGroupTooltip>
                  {getString('connectWalletCustodialTooltip')}
                </style.providerGroupTooltip>
              </div>
            </style.providerGroupHeaderIcon>
          </style.providerGroupHeader>
          <style.providerGroup>
            {props.providers.map(renderProviderButton)}
          </style.providerGroup>
          <style.regionsLearnMore>
            <NewTabLink href={urls.supportedWalletRegionsURL}>
              {getString('connectWalletLearnMore')}
            </NewTabLink>
          </style.regionsLearnMore>
        </style.providerSelection>
      </style.content>
    </style.root>
  )
}
