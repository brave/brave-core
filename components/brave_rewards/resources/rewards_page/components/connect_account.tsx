/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import ProgressRing from '@brave/leo/react/progressRing'
import Tooltip from '@brave/leo/react/tooltip'

import { AppModelContext, useAppState } from '../lib/app_model_context'
import { RouterContext } from '../lib/router'
import { formatMessage } from '../../shared/lib/locale_context'
import { useCallbackWrapper } from '../lib/callback_wrapper'
import { useLocaleContext } from '../lib/locale_strings'
import { NewTabLink } from '../../shared/components/new_tab_link'
import { WalletProviderIcon } from '../../shared/components/icons/wallet_provider_icon'

import {
  ExternalWalletProvider,
  isSelfCustodyProvider,
  getExternalWalletProviderName,
  isExternalWalletProviderAllowed
} from '../../shared/lib/external_wallet'

import * as routes from '../lib/app_routes'
import * as urls from '../../shared/lib/rewards_urls'

import { style } from './connect_account.style'

type LoadingState = 'loading' | 'error' | ''

export function ConnectAccount() {
  const model = React.useContext(AppModelContext)
  const router = React.useContext(RouterContext)
  const { getString } = useLocaleContext()
  const wrapCallback = useCallbackWrapper()

  const [
    countryCode,
    regions,
    providers,
    externalWallet
  ] = useAppState((state) => [
    state.countryCode,
    state.rewardsParameters?.walletProviderRegions ?? null,
    state.externalWalletProviders,
    state.externalWallet
  ])

  const [loadingState, setLoadingState] = React.useState<LoadingState>('')
  const [selectedProvider, setSelectedProvider] =
    React.useState<ExternalWalletProvider | null>(null)

  React.useEffect(() => {
    // If the user has connected an external wallet but is not longer logged in,
    // then automatically start the login process.
    if (externalWallet) {
      if (!externalWallet.authenticated) {
        model.beginExternalWalletLogin(externalWallet.provider)
      } else {
        router.replaceRoute(routes.home)
      }
    }
  }, [])

  function onBack() {
    router.setRoute(routes.home)
  }

  function providerButtonText(provider: ExternalWalletProvider) {
    if (provider === 'solana') {
      return getString('connectSolanaButtonLabel')
    }
    return getExternalWalletProviderName(provider)
  }

  function providerButtonMessage(
    provider: ExternalWalletProvider,
    allowed: boolean
  ) {
    if (!allowed) {
      return (
        <span className='message'>
          {getString('connectProviderNotAvailable')}
        </span>
      )
    }
    if (provider === 'solana') {
      return (
        <span className='message'>
          {getString('connectSolanaMessage')}
        </span>
      )
    }
    return null
  }

  function providerIcon(provider: ExternalWalletProvider) {
    if (provider === 'solana') {
      return (
        <>
          <Icon name='brave-icon-release-color' />
          <div className='new-badge'>{getString('newBadgeText')}</div>
        </>
      )
    }
    return <WalletProviderIcon provider={provider} />
  }

  function providerButtonCaret(provider: ExternalWalletProvider) {
    if (isSelfCustodyProvider(provider)) {
      if (selectedProvider === provider && loadingState === 'loading') {
        return (
          <span className='caret'>
            <ProgressRing />
          </span>
        )
      }
      return null
    }
    return (
      <span className='caret'>
        {getString('connectLoginText')}
        <Icon name='carat-right' />
      </span>    )
  }

  function providerButton(provider: ExternalWalletProvider) {
    const allowed = isExternalWalletProviderAllowed(
      countryCode,
      regions && regions[provider] || null)

    const onClick = () => {
      if (allowed) {
        setSelectedProvider(provider)
        setLoadingState('loading')
        model.beginExternalWalletLogin(provider).then(wrapCallback((ok) => {
          setLoadingState(ok ? '' : 'error')
        }))
      }
    }

    return (
      <button
        data-provider={provider}
        key={provider}
        disabled={!allowed}
        onClick={onClick}
      >
        {providerIcon(provider)}
        <span className='name'>
          {providerButtonText(provider)}
          {providerButtonMessage(provider, allowed)}
        </span>
        {allowed && providerButtonCaret(provider)}
      </button>
    )
  }

  function renderCustodialSection() {
    if (!providers) {
      return null
    }

    const entries = providers.filter((name) => !isSelfCustodyProvider(name))
    if (entries.length === 0) {
      return null
    }

    return (
      <>
        <h3>
          {getString('connectCustodialTitle')}
          <Tooltip mode='default'>
            <Icon name='info-outline' />
            <div slot='content'>
              {getString('connectCustodialTooltip')}
            </div>
          </Tooltip>
        </h3>
        <section>
          {entries.map(providerButton)}
        </section>
        <p className='regions-learn-more'>
          <NewTabLink href={urls.supportedWalletRegionsURL}>
            {getString('connectRegionsLearnMoreText')}
          </NewTabLink>
        </p>
      </>
    )
  }

  function renderSelfCustodySection() {
    if (!providers) {
      return null
    }

    const entries = providers.filter(isSelfCustodyProvider)
    if (entries.length === 0) {
      return null
    }

    return (
      <>
        <h3>
          {getString('connectSelfCustodyTitle')}
          <Tooltip mode='default'>
            <Icon name='info-outline' />
            <div slot='content'>
              {getString('connectSelfCustodyTooltip')}
              <div className='self-custody-learn-more'>
                <NewTabLink href={urls.selfCustodyLearnMoreURL}>
                  {getString('learnMoreLink')}
                </NewTabLink>
              </div>
            </div>
          </Tooltip>
        </h3>
        <section>
          {entries.map(providerButton)}
        </section>
        {
          loadingState === 'error' &&
          selectedProvider &&
          isSelfCustodyProvider(selectedProvider) &&
            <div className='connect-error'>
              <Icon name='warning-triangle-filled' />
              <span>{getString('connectSelfCustodyError')}</span>
            </div>
        }
        <p className='self-custody-note'>
          {getString('connectSelfCustodyNote')}{' '}
          {
            formatMessage(getString('connectSelfCustodyTerms'), {
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
        </p>
      </>
    )
  }

  // When reconnecting an external wallet do not show any UI. The user will be
  // automatically redirected to the provider's login flow.
  if (externalWallet) {
    return null
  }

  return (
    <div {...style}>
      <div className='brave-rewards-logo' />
      <nav>
        <Button kind='outline' size='small' fab onClick={onBack}>
          <Icon name='arrow-left' />
        </Button>
      </nav>
      <h1>{getString('connectTitle')}</h1>
      <p className='text'>{getString('connectText')}</p>
      {renderCustodialSection()}
      {renderSelfCustodySection()}
    </div>
  )
}
