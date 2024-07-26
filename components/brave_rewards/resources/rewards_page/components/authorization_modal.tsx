/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import ProgressRing from '@brave/leo/react/progressRing'

import { RouterContext } from '../lib/router'
import { ConnectExternalWalletResult } from '../lib/app_model'
import { AppModelContext } from '../lib/app_model_context'
import { formatMessage } from '../../shared/lib/locale_context'
import { useLocaleContext } from '../lib/locale_strings'
import { useCallbackWrapper } from '../lib/callback_wrapper'
import { NewTabLink } from '../../shared/components/new_tab_link'
import { Modal } from './modal'
import * as routes from '../lib/app_routes'
import * as urls from '../../shared/lib/rewards_urls'

import {
  externalWalletProviderFromString,
  getExternalWalletProviderName
} from '../../shared/lib/external_wallet'

import { style } from './authorization_modal.style'

export function AuthorizationModal() {
  const router = React.useContext(RouterContext)
  const model = React.useContext(AppModelContext)
  const { getString } = useLocaleContext()
  const wrapCallback = useCallbackWrapper()

  const [result, setResult] =
    React.useState<ConnectExternalWalletResult | null>(null)

  function getProvider() {
    // After OAuth login, the browser will be redirected to
    // "//rewards/<provider>/authorization"; the provider name is the first
    // path component.
    const [, providerString] = location.pathname.split(/\//g)
    return externalWalletProviderFromString(providerString)
  }

  function onClose() {
    router.replaceRoute(routes.home)
  }

  React.useEffect(() => {
    const provider = getProvider()
    if (!provider) {
      console.error('Missing provider name in path')
      setResult('unexpected-error')
      return
    }

    const params = new URLSearchParams(location.search)
    const args = Object.fromEntries(params.entries())

    model.connectExternalWallet(provider, args).then(wrapCallback((result) => {
      if (result === 'success') {
        onClose()
      } else {
        setResult(result)
      }
    }))
  }, [model])

  function errorTitle() {
    switch (result) {
      case null:
      case 'success':
        return ''
      case 'device-limit-reached':
        return getString('authorizeDeviceLimitReachedTitle')
      case 'flagged-wallet':
        return getString('authorizeFlaggedWalletTitle')
      case 'kyc-required':
        return getString('authorizeKycRequiredTitle')
      case 'mismatched-countries':
        return getString('authorizeMismatchedCountriesTitle')
      case 'mismatched-provider-accounts':
        return getString('authorizeMismatchedProviderAccountsTitle')
      case 'provider-unavailable':
        return getString('authorizeProviderUnavailableTitle')
      case 'region-not-supported':
        return getString('authorizeRegionNotSupportedTitle')
      case 'request-signature-verification-error':
      case 'uphold-transaction-verification-failure':
        return getString('authorizeSignatureVerificationErrorTitle')
      case 'uphold-bat-not-allowed':
        return getString('authorizeUpholdBatNotAllowedTitle')
      case 'uphold-insufficient-capabilities':
        return getString('authorizeUpholdInsufficientCapabilitiesTitle')
      case 'unexpected-error':
        return getString('authorizeUnexpectedErrorTitle')
    }
  }

  function errorText(): React.ReactNode {
    const provider = getProvider()
    const providerName = provider ? getExternalWalletProviderName(provider) : ''

    switch (result) {
      case null:
      case 'success':
        return ''
      case 'device-limit-reached':
        return formatMessage(getString('authorizeDeviceLimitReachedText'), [
          providerName
        ])
      case 'flagged-wallet':
        return <>
          <p>
            {getString('authorizeFlaggedWalletText1')}
            {getString('authorizeFlaggedWalletText2')}
          </p>
          <p>{getString('authorizeFlaggedWalletText3')}</p>
          <p>
            <NewTabLink href={urls.flaggedWalletURL}>
              {getString('authorizeFlaggedWalletText4')}
            </NewTabLink>
          </p>
        </>
      case 'kyc-required':
        return formatMessage(getString('authorizeKycRequiredText'), [
          providerName
        ])
      case 'mismatched-countries':
        return <>
          <p>
            {
              formatMessage(getString('authorizeMismatchedCountriesText'), [
                providerName
              ])
            }
          </p>
          <p>
            <NewTabLink href={urls.mismatchedCountriesURL}>
              {getString('learnMoreLink')}
            </NewTabLink>
          </p>
        </>
      case 'mismatched-provider-accounts':
        return <>
          <p>
            {
              formatMessage(
                getString('authorizeMismatchedProviderAccountsText'),
                [providerName])
            }
          </p>
          <p>
            <NewTabLink href={urls.mismatchedProviderAccountsURL}>
              {getString('learnMoreLink')}
            </NewTabLink>
          </p>
        </>
      case 'provider-unavailable':
        return <>
          <p>
            {
              formatMessage(getString('authorizeProviderUnavailableText1'), [
                providerName
              ])
            }
          </p>
          <p>
            {
              formatMessage(getString('authorizeProviderUnavailableText2'), {
                tags: {
                  $2: (content) => (
                    <NewTabLink key='link' href={urls.braveStatusURL}>
                      {content}
                    </NewTabLink>
                  )
                }
              })
            }
          </p>
        </>
      case 'region-not-supported':
        return <>
          <p>
            {
              formatMessage(getString('authorizeRegionNotSupportedText1'), [
                providerName
              ])
            }
          </p>
          <p>
            {
              formatMessage(getString('authorizeRegionNotSupportedText2'), {
                tags: {
                  $2: (content) => (
                    <NewTabLink
                      key='link'
                      href={urls.supportedWalletRegionsURL}
                    >
                      {content}
                    </NewTabLink>
                  )
                }
              })
            }
          </p>
        </>
      case 'request-signature-verification-error':
      case 'uphold-transaction-verification-failure':
        return (
          <p>
            {
              formatMessage(
                getString('authorizeSignatureVerificationErrorText'),
                {
                  placeholders: {
                    $1: providerName
                  },
                  tags: {
                    $2: (content) => (
                      <NewTabLink key='link' href={urls.contactSupportURL}>
                        {content}
                      </NewTabLink>
                    )
                  }
                })
            }
          </p>
        )
      case 'uphold-bat-not-allowed':
        return getString('authorizeUpholdBatNotAllowedText')
      case 'uphold-insufficient-capabilities':
        return getString('authorizeUpholdInsufficientCapabilitiesText')
      case 'unexpected-error':
        return (
          <p>
            {
              formatMessage(getString('authorizeUnexpectedErrorText'), {
                tags: {
                  $2: (content) => (
                    <NewTabLink key='link' href={urls.contactSupportURL}>
                      {content}
                    </NewTabLink>
                  )
                }
              })
            }
          </p>
        )
    }
  }

  if (!result) {
    return (
      <Modal>
        <div {...style}>
          <div className='processing'>
            <ProgressRing />
            {getString('authorizeProcessingText')}
          </div>
        </div>
      </Modal>
    )
  }

  return (
    <Modal className='authorization-modal' onEscape={onClose}>
      <Modal.Header
        title={getString('authorizeErrorTitle')}
        onClose={onClose}
      />
      <div {...style}>
        <div className='status-icon'>
          <Icon name='warning-triangle-filled' />
        </div>
        <h3>{errorTitle()}</h3>
        <div className='error-text'>{errorText()}</div>
      </div>
      <Modal.Actions
        actions={[
          { text: getString('closeButtonLabel'), onClick: onClose }
        ]}
      />
    </Modal>
  )
}
