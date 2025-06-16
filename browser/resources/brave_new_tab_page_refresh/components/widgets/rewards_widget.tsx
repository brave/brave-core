/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import { getString } from '../../lib/strings'
import { useRewardsState } from '../../context/rewards_context'
import { Link, openLink } from '../common/link'

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

  if (!rewardsEnabled) {
    return renderOnboarding()
  }

  if (!externalWallet) {
    return renderUnconnected()
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
                <span className='exchange'>
                  ≈ {
                    exchangeAmountFormatter.format(
                        balance * exchangeRate)
                  } USD
                </span>
              </>
            }
          </div>
        </div>
      </div>
    </div>
  )
}
