/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'
import ProgressRing from '@brave/leo/react/progressRing'

import { LocaleContext, formatMessage } from '../../shared/lib/locale_context'
import { NewTabLink } from '../../shared/components/new_tab_link'
import { WalletProviderIcon } from '../../shared/components/icons/wallet_provider_icon'
import { BraveLogoText } from './icons/brave_logo_text'

import {
  ExternalWalletProvider,
  isSelfCustodyProvider,
  getExternalWalletProviderName
} from '../../shared/lib/external_wallet'

import * as urls from '../../shared/lib/rewards_urls'
import * as style from './connect_wallet_modal.style'

interface ProviderInfo {
  provider: ExternalWalletProvider
  enabled: boolean
}

interface Props {
  currentCountryCode: string
  providers: ProviderInfo[]
  connectState: 'loading' | 'error' | ''
  onContinue: (provider: string) => void
  onClose: () => void
}

export function ConnectWalletModal (props: Props) {
  const { getString } = React.useContext(LocaleContext)
  const [selectedProvider, setSelectedProvider] =
    React.useState<ExternalWalletProvider | null>(null)

  React.useEffect(() => {
    // While this overlay is displayed, hide any scrollbars attached to the
    // document body, as that can result in two visible scrollbars on some
    // platforms.
    document.body.style.overflow = 'hidden';
    return () => { document.body.style.overflow = ''; }
  }, []);

  if (props.providers.length === 0) {
    return null
  }

  function renderCustodialProviders () {
    const entries = props.providers.filter(
      (info => !isSelfCustodyProvider(info.provider)))

    if (entries.length === 0) {
      return null
    }

    return (
      <div>
        <style.providerGroupHeader>
          <span>{getString('connectWalletCustodialHeader')}</span>
          <style.providerGroupHeaderIcon>
            <Icon name='info-outline' />
            <div className='custodial tooltip'>
              <style.providerGroupTooltip>
                {getString('connectWalletCustodialTooltip')}
              </style.providerGroupTooltip>
            </div>
          </style.providerGroupHeaderIcon>
        </style.providerGroupHeader>
        <style.providerGroupItems>
          {entries.map(renderProviderButton)}
        </style.providerGroupItems>
        <style.regionsLearnMore>
          <NewTabLink href={urls.supportedWalletRegionsURL}>
            {getString('connectWalletLearnMore')}
          </NewTabLink>
        </style.regionsLearnMore>
      </div>
    )
  }

  function renderSelfCustodyProviders () {
    const entries = props.providers.filter(
      (info => isSelfCustodyProvider(info.provider)))

    if (entries.length === 0) {
      return null
    }

    return (
      <div>
        <style.providerGroupHeader>
          {getString('connectWalletSelfCustodyHeader')}
          <style.providerGroupHeaderIcon>
            <Icon name='info-outline' />
            <div className='self-custody tooltip'>
              <style.providerGroupTooltip>
                <div>
                  {getString('connectWalletSelfCustodyTooltip')}
                </div>
                <div>
                  <NewTabLink href={urls.selfCustodyLearnMoreURL}>
                    {getString('learnMore')}
                  </NewTabLink>
                </div>
              </style.providerGroupTooltip>
            </div>
          </style.providerGroupHeaderIcon>
        </style.providerGroupHeader>
        <style.providerGroupItems>
          {entries.map(renderProviderButton)}
        </style.providerGroupItems>
        {
          props.connectState === 'error' &&
            <style.connectError>
              <Icon name='warning-triangle-filled' />
              <span>{getString('connectWalletSelfCustodyError')}</span>
            </style.connectError>
        }
        <style.selfCustodyNote>
          {getString('connectWalletSelfCustodyNote')}
          {' '}
          {
            formatMessage(getString('connectWalletSelfCustodyTerms'), {
              tags: {
                $1: (content) => (
                  <NewTabLink key='terms' href={urls.termsOfServiceURL}>
                    {content}
                  </NewTabLink>
                ),
                $3: (content) => (
                  <NewTabLink key='privacy-policy' href={urls.privacyPolicyURL}>
                    {content}
                  </NewTabLink>
                )
              }
            })
          }
        </style.selfCustodyNote>
      </div>
    )
  }

  function renderProviderIcon (provider: ExternalWalletProvider) {
    if (provider === 'solana') {
      return (
        <>
          <Icon name='brave-icon-release-color' />
          <style.newBadge>{getString('newBadgeText')}</style.newBadge>
        </>
      )
    }
    return <WalletProviderIcon provider={provider} />
  }

  function providerButtonText (provider: ExternalWalletProvider) {
    if (provider === 'solana') {
      return getString('connectWalletSolanaLabel')
    }
    return getExternalWalletProviderName(provider)
  }

  function providerButtonMessage (providerInfo: ProviderInfo) {
    if (!providerInfo.enabled) {
      return (
        <style.providerButtonMessage>
          {getString('connectWalletProviderNotAvailable')}
        </style.providerButtonMessage>
      )
    }
    if (providerInfo.provider === 'solana') {
      return (
        <style.providerButtonMessage>
          {getString('connectWalletSolanaMessage')}
        </style.providerButtonMessage>
      )
    }
    return null
  }

  function providerButtonCaret (providerInfo: ProviderInfo) {
    if (!providerInfo.enabled) {
      return null
    }
    if (isSelfCustodyProvider(providerInfo.provider)) {
      if (selectedProvider === providerInfo.provider &&
          props.connectState === 'loading') {
        return (
          <style.providerButtonCaret>
            <ProgressRing />
          </style.providerButtonCaret>
        )
      }
      return null
    }
    return (
      <style.providerButtonCaret>
        <style.providerButtonCaretText>
          {getString('connectWalletLoginText')}
        </style.providerButtonCaretText>
        <Icon name='carat-right' />
      </style.providerButtonCaret>
    )
  }

  function renderProviderButton (providerInfo: ProviderInfo) {
    const { provider, enabled } = providerInfo
    const onClick = () => {
      if (enabled) {
        setSelectedProvider(provider)
        props.onContinue(provider)
      }
    }

    return (
      <button
        data-test-id='connect-provider-button'
        key={provider}
        onClick={onClick}
        disabled={!enabled}
      >
        <style.providerButtonIcon>
          {renderProviderIcon(provider)}
        </style.providerButtonIcon>
        <style.providerButtonName>
          {providerButtonText(provider)}
          {providerButtonMessage(providerInfo)}
        </style.providerButtonName>
        {providerButtonCaret(providerInfo)}
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
        <style.providerGroups>
          {renderCustodialProviders()}
          {renderSelfCustodyProviders()}
        </style.providerGroups>
      </style.content>
    </style.root>
  )
}
