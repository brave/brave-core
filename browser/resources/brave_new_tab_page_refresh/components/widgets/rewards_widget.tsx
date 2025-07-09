/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import Tooltip from '@brave/leo/react/tooltip'

import { getString } from '../../lib/strings'
import { useRewardsState } from '../../context/rewards_context'
import { usePluralString } from '../../lib/plural_string'
import { Link, openLink } from '../common/link'
import { WalletProviderIcon } from '../../../../../components/brave_rewards/resources/shared/components/icons/wallet_provider_icon'
import { getExternalWalletProviderName } from '../../../../../components/brave_rewards/resources/shared/lib/external_wallet'
import formatMessage from '$web-common/formatMessage'

import * as urls from '../../../../../components/brave_rewards/resources/shared/lib/rewards_urls'

import { style } from './rewards_widget.style'

const batAmountFormatter = new Intl.NumberFormat(undefined, {
  minimumFractionDigits: 2,
  maximumFractionDigits: 4
})

const exchangeAmountFormatter = new Intl.NumberFormat(undefined, {
  minimumFractionDigits: 2,
  maximumFractionDigits: 2
})

export function RewardsWidget() {
  const rewardsEnabled = useRewardsState((s) => s.rewardsEnabled)
  const externalWallet = useRewardsState((s) => s.rewardsExternalWallet)
  const balance = useRewardsState((s) => s.rewardsBalance)
  const exchangeRate = useRewardsState((s) => s.rewardsExchangeRate)
  const adsViewed = useRewardsState((s) => s.rewardsAdsViewed)
  const adsViewedString =
    usePluralString('rewardsConnectedAdsViewedText', adsViewed ?? 0)

  function renderOnboarding() {
    return (
      <div data-css-scope={style.scope} className='onboarding'>
        <div className='title'>
          <Icon name='product-bat-color' /> {getString('rewardsWidgetTitle')}
        </div>
        <div className='content'>
          <div className='text'>
            <div>
              <Icon name='check-normal' />
              <div>{getString('rewardsFeatureText1')}</div>
            </div>
            <div>
              <Icon name='check-normal' />
              <div>{getString('rewardsFeatureText2')}</div>
            </div>
          </div>
          <div className='actions'>
            <Button
              size='small'
              onClick={() => openLink(urls.settingsURL)}
            >
              {getString('rewardsOnboardingButtonLabel')}
            </Button>
            <Link url={urls.rewardsTourURL}>
              {getString('rewardsOnboardingLink')}
            </Link>
          </div>
        </div>
      </div>
    )
  }

  function renderUnconnected() {
    return (
      <div data-css-scope={style.scope} className='unconnected'>
        <div className='title'>
          {getString('rewardsWidgetTitle')}
        </div>
        <div className='content'>
          <div className='connect-graphic' />
          <div className='text'>
            <div className='header'>
              {getString('rewardsConnectTitle')}
            </div>
            <div>
              {getString('rewardsConnectText')}
            </div>
          </div>
          <div className='actions'>
            <Button
              size='small'
              onClick={() => openLink(urls.connectURL)}
            >
              {getString('rewardsConnectButtonLabel')}
            </Button>
          </div>
        </div>
      </div>
    )
  }

  function renderLogin() {
    if (!externalWallet) {
      return null
    }
    return (
      <div data-css-scope={style.scope} className='login'>
        <div className='title'>
          {getString('rewardsWidgetTitle')}
        </div>
        <div className='content'>
          <Icon name='bat-color' />
          <div className='text'>
            <div className='header'>
              {getString('rewardsLoginTitle')}
            </div>
            <div>
              {
                formatMessage(getString('rewardsLoginText'), [
                  getExternalWalletProviderName(externalWallet.provider)
                ])
              }
            </div>
          </div>
          <div className='actions'>
            <Button
              size='small'
              onClick={() => openLink(urls.reconnectURL)}
            >
              <span slot='icon-before'>
                <WalletProviderIcon provider={externalWallet.provider} />
              </span>
              {getString('rewardsLoginButtonLabel')}
            </Button>
          </div>
        </div>
      </div>
    )
  }

  if (!rewardsEnabled) {
    return renderOnboarding()
  }

  if (!externalWallet) {
    return renderUnconnected()
  }

  if (!externalWallet.authenticated) {
    return renderLogin()
  }

  return (
    <div data-css-scope={style.scope} className='connected'>
      <div className='title'>
        {getString('rewardsWidgetTitle')}
      </div>
      <div className='content'>
        <div className='coin-graphic' />
        <div className='text'>
          <div className='header'>
            {getString('rewardsBalanceTitle')}
          </div>
          <div className='balance'>
            {
              balance !== null && <>
                <span className='bat-amount'>
                  {batAmountFormatter.format(balance)}
                </span>
                <span className='bat-label'>BAT</span>
                {
                  exchangeRate &&
                    <span className='exchange'>
                      ≈ {
                      exchangeAmountFormatter.format(balance * exchangeRate)
                      } USD
                    </span>
                }
              </>
            }
          </div>
          {
            adsViewed !== null &&
              <div className='ads-viewed'>
                {
                  formatMessage(adsViewedString, {
                    tags: {
                      $1: (content) => (
                        <span key='ad-count' className='ad-count'>
                          {content}
                        </span>
                      )
                    }
                  })
                }
                <Tooltip mode='default'>
                  <Icon name='info-outline' />
                  <div slot='content'>
                    {getString('rewardsAdsViewedTooltip')}
                  </div>
                </Tooltip>
              </div>
          }
        </div>
      </div>
    </div>
  )
}
