/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import Tooltip from '@brave/leo/react/tooltip'

import { getString } from '../../lib/strings'
import { useRewardsState, useRewardsActions } from '../../context/rewards_context'
import { usePluralString } from '../../lib/plural_string'
import { usePersistedJSON } from '$web-common/usePersistedState'
import { WidgetMenu } from './widget_menu'
import { Link, openLink } from '../common/link'
import { WalletProviderIcon } from '../../../../../components/brave_rewards/resources/shared/components/icons/wallet_provider_icon'
import { getExternalWalletProviderName } from '../../../../../components/brave_rewards/resources/shared/lib/external_wallet'
import { getProviderPayoutStatus } from '../../../../../components/brave_rewards/resources/shared/lib/provider_payout_status'
import { formatString } from '$web-common/formatString'

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

const monthNameFormatter = new Intl.DateTimeFormat(undefined, {
  month: 'long'
})

function getPayoutMonth() {
  const date = new Date()
  const lastMonth = new Date(date.getFullYear(), date.getMonth() - 1)
  return monthNameFormatter.format(lastMonth)
}

export function RewardsWidget() {
  const rewardsEnabled = useRewardsState((s) => s.rewardsEnabled)
  const externalWallet = useRewardsState((s) => s.rewardsExternalWallet)
  const rewardsBalance = useRewardsState((s) => s.rewardsBalance)
  const exchangeRate = useRewardsState((s) => s.rewardsExchangeRate)
  const payoutStatus = useRewardsState((s) => s.payoutStatus)
  const earnings = useRewardsState((s) => s.minEarningsPreviousMonth)
  const tosUpdateRequired = useRewardsState((s) => s.tosUpdateRequired)
  const adsViewed = useRewardsState((s) => s.rewardsAdsViewed)
  const adsViewedString =
    usePluralString('rewardsConnectedAdsViewedText', adsViewed ?? 0)
  const [cachedBalance, setCachedBalance] = usePersistedJSON<number | null>(
    'ntp-rewards-balance',
    (data) => typeof data === 'number' ? data : null)

  React.useEffect(() => {
    if (rewardsBalance !== null) {
      setCachedBalance(rewardsBalance)
    }
  }, [rewardsBalance])

  function renderOnboarding() {
    return (
      <RewardsWidgetContainer className='onboarding'>
        <div className='title'>
          <Icon name='product-bat-color' />
          {getString(S.NEW_TAB_REWARDS_WIDGET_TITLE)}
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
              {getString(S.NEW_TAB_REWARDS_ONBOARDING_BUTTON_LABEL)}
            </Button>
            <Link url={urls.rewardsTourURL}>
              {getString(S.NEW_TAB_REWARDS_ONBOARDING_LINK)}
            </Link>
          </div>
        </div>
      </RewardsWidgetContainer>
    )
  }

  function renderUnconnected() {
    return (
      <RewardsWidgetContainer className='unconnected'>
        <div className='title'>
          {getString(S.NEW_TAB_REWARDS_WIDGET_TITLE)}
        </div>
        <div className='content'>
          <div className='connect-graphic' />
          <div className='text'>
            <div className='header'>
              {getString(S.NEW_TAB_REWARDS_CONNECT_TITLE)}
            </div>
            <div>
              {getString(S.NEW_TAB_REWARDS_CONNECT_TEXT)}
            </div>
          </div>
          <div className='actions'>
            <Button
              size='small'
              onClick={() => openLink(urls.connectURL)}
            >
              {getString(S.NEW_TAB_REWARDS_CONNECT_BUTTON_LABEL)}
            </Button>
          </div>
        </div>
      </RewardsWidgetContainer>
    )
  }

  function renderLogin() {
    if (!externalWallet) {
      return null
    }
    return (
      <RewardsWidgetContainer className='login'>
        <div className='title'>
          {getString(S.NEW_TAB_REWARDS_WIDGET_TITLE)}
        </div>
        <div className='content'>
          <Icon name='bat-color' />
          <div className='text'>
            <div className='header'>
              {getString(S.NEW_TAB_REWARDS_LOGIN_TITLE)}
            </div>
            <div>
              {
                formatString(getString(S.NEW_TAB_REWARDS_LOGIN_TEXT), [
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
              {getString(S.NEW_TAB_REWARDS_LOGIN_BUTTON_LABEL)}
            </Button>
          </div>
        </div>
      </RewardsWidgetContainer>
    )
  }

  function renderTosUpdateNotice() {
    return (
      <RewardsWidgetContainer className='login'>
        <div className='title'>
          {getString(S.NEW_TAB_REWARDS_WIDGET_TITLE)}
        </div>
        <div className='content'>
          <div className='text'>
            <div className='header'>
              {getString('rewardsTosUpdateTitle')}
            </div>
            <div>
              {getString('rewardsTosUpdateText')}
            </div>
          </div>
          <div className='actions'>
            <Button
              size='small'
              onClick={() => openLink(urls.settingsURL)}
            >
              {getString('rewardsTosUpdateButtonLabel')}
            </Button>
          </div>
        </div>
      </RewardsWidgetContainer>
    )
  }

  function renderPayoutStatus() {
    if (earnings <= 0 || !externalWallet) {
      return null
    }
    const status =
      getProviderPayoutStatus(payoutStatus, externalWallet.provider)
    if (status === 'complete') {
      return (
        <div className='payout-status'>
          {
            formatString(getString('rewardsPayoutCompletedText'), [
              getPayoutMonth()
            ])
          }
        </div>
      )
    }
    if (status === 'processing') {
      return (
        <div className='payout-status'>
          {
            formatString(getString('rewardsPayoutProcessingText'), [
              getPayoutMonth()
            ])
          }
          <Link url={urls.payoutStatusURL}>
            {getString(S.NEW_TAB_REWARDS_PAYOUT_DETAILS_LINK)}
          </Link>
        </div>
      )
    }
    return null
  }

  function renderAdsViewed() {
    if (adsViewed === null) {
      return null
    }
    return (
      <div className='ads-viewed'>
        {
          adsViewedString && formatString(adsViewedString, {
            $1: (content) => (
              <span className='ad-count'>
                {content}
              </span>
            )
          })
        }
        <Tooltip mode='default'>
          <Icon name='info-outline' />
          <div slot='content'>
            {getString('rewardsAdsViewedTooltip')}
          </div>
        </Tooltip>
      </div>
    )
  }

  function renderBalance() {
    const balance = rewardsBalance ?? cachedBalance
    if (balance === null) {
      return <div className='balance skeleton' />
    }
    return (
      <div className='balance'>
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
      </div>
    )
  }

  if (!rewardsEnabled) {
    return renderOnboarding()
  }

  if (tosUpdateRequired) {
    return renderTosUpdateNotice()
  }

  if (!externalWallet) {
    return renderUnconnected()
  }

  if (!externalWallet.authenticated) {
    return renderLogin()
  }

  return (
    <RewardsWidgetContainer className='connected'>
      <div className='title'>
        {getString(S.NEW_TAB_REWARDS_WIDGET_TITLE)}
      </div>
      <div className='content'>
        <div className='coin-graphic' />
        <div className='text'>
          <div className='header'>
            {getString(S.NEW_TAB_REWARDS_BALANCE_TITLE)}
          </div>
          {renderBalance()}
          {renderPayoutStatus() || renderAdsViewed()}
        </div>
      </div>
    </RewardsWidgetContainer>
  )
}

interface ContainerProps {
  className: string
  children: React.ReactNode
}

function RewardsWidgetContainer(props: ContainerProps) {
  const actions = useRewardsActions()
  return (
    <div data-css-scope={style.scope} className={props.className}>
      <WidgetMenu>
        <leo-menu-item onClick={() => actions.setShowRewardsWidget(false)}>
          <Icon name='eye-off' />
          {getString(S.NEW_TAB_HIDE_REWARDS_WIDGET_LABEL)}
        </leo-menu-item>
      </WidgetMenu>
      {props.children}
    </div>
  )
}
