/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext, formatMessage } from '../../lib/locale_context'
import { GrantInfo } from '../../lib/grant_info'
import { BatIcon } from '../icons/bat_icon'
import { SettingsIcon } from '../icons/settings_icon'
import { CloseIcon } from '../icons/close_icon'
import { MoneyBagIcon } from '../icons/money_bag_icon'
import { InfoIcon } from './icons/info_icon'
import { TermsOfService } from '../terms_of_service'
import { AsyncButton } from './async_button'
import { TokenAmount } from '../token_amount'
import { ExchangeAmount } from '../exchange_amount'
import { NewTabLink } from '../new_tab_link'
import { getDaysUntilRewardsPayment } from '../../lib/pending_rewards'

import * as urls from '../../lib/rewards_urls'

import * as style from './rewards_card.style'

const nextPaymentDateFormatter = new Intl.DateTimeFormat(undefined, {
  day: 'numeric',
  month: 'short'
})

const monthDayFormatter = new Intl.DateTimeFormat(undefined, {
  day: 'numeric',
  month: 'short'
})

function renderDateRange () {
  const now = new Date()
  const start = new Date(now.getFullYear(), now.getMonth(), 1)
  const end = new Date(start.getFullYear(), start.getMonth() + 1, -1)
  return (
    <>{monthDayFormatter.format(start)} - {monthDayFormatter.format(end)}</>
  )
}

function getGrantMessages (grantInfo: GrantInfo) {
  if (grantInfo.type === 'ads') {
    return {
      title: 'rewardsAdGrantTitle',
      text: 'rewardsAdGrantText',
      button: 'rewardsClaimEarnings'
    }
  }

  return {
    title: 'rewardsTokenGrantTitle',
    text: 'rewardsTokenGrantText',
    button: 'rewardsClaimTokens'
  }
}

export function RewardsCardHeader () {
  const { getString } = React.useContext(LocaleContext)
  return (
    <style.cardHeader>
      <BatIcon />
      <style.cardHeaderText>
        {getString('rewardsBraveRewards')}
      </style.cardHeaderText>
    </style.cardHeader>
  )
}

interface Props {
  rewardsEnabled: boolean
  adsEnabled: boolean
  adsSupported: boolean
  rewardsBalance: number
  exchangeRate: number
  exchangeCurrency: string
  nextPaymentDate: number
  earningsThisMonth: number
  earningsLastMonth: number
  contributionsThisMonth: number
  grantInfo: GrantInfo | null
  onEnableRewards: () => void
  onEnableAds: () => void
  onDismissGrant: () => void
  onClaimGrant: () => void
}

export function RewardsCard (props: Props) {
  const { getString } = React.useContext(LocaleContext)

  function renderGrantOverlay () {
    const { grantInfo } = props
    if (!grantInfo) {
      return null
    }

    const createdAt = grantInfo.createdAt || 0
    const grantDate = createdAt > 0
      ? monthDayFormatter.format(createdAt).toLocaleUpperCase()
      : ''

    const messages = getGrantMessages(grantInfo)

    return (
      <style.grant>
        <style.grantHeader>
          <div>
            <MoneyBagIcon />{getString(messages.title)}
          </div>
          <div>
            <button onClick={props.onDismissGrant}>
              <CloseIcon />
            </button>
          </div>
        </style.grantHeader>
        <style.grantText>
          <div>
            {
              formatMessage(getString(messages.text), [
                <TokenAmount
                  key='amount'
                  amount={grantInfo.amount}
                  minimumFractionDigits={1}
                />
              ])
            }
          </div>
          {grantDate && <style.grantDate>{grantDate}</style.grantDate>}
        </style.grantText>
        <style.primaryAction>
          <button onClick={props.onClaimGrant}>
            {getString(messages.button)}
          </button>
        </style.primaryAction>
      </style.grant>
    )
  }

  function renderBalance () {
    if (props.grantInfo) {
      return renderGrantOverlay()
    }

    const pendingDays = props.earningsLastMonth > 0
      ? getDaysUntilRewardsPayment(props.nextPaymentDate)
      : ''

    return (
      <style.balance>
        <style.balanceTitle>
          {getString('rewardsTokenBalance')}
        </style.balanceTitle>
        <style.balanceAmount>
          <TokenAmount amount={props.rewardsBalance} />
        </style.balanceAmount>
        <style.balanceExchange>
          <style.balanceExchangeAmount>
            ≈&nbsp;
            <ExchangeAmount
              amount={props.rewardsBalance}
              rate={props.exchangeRate}
              currency={props.exchangeCurrency}
            />
          </style.balanceExchangeAmount>
          {
            props.rewardsBalance > 0 && !pendingDays &&
              <style.balanceExchangeNote>
                {getString('rewardsExchangeValueNote')}
              </style.balanceExchangeNote>
          }
        </style.balanceExchange>
        {
          pendingDays &&
            <style.pendingRewards>
              {
                formatMessage(getString('rewardsPendingPayoutMessage'), [
                  <span key='amount'>
                    <strong>+</strong>
                    <TokenAmount
                      amount={props.earningsLastMonth}
                      minimumFractionDigits={1}
                    />
                  </span>,
                  pendingDays
                ])
              }
            </style.pendingRewards>
        }
      </style.balance>
    )
  }

  function renderRewardsOptIn () {
    return (
      <style.root>
        <RewardsCardHeader />
        <style.rewardsOptIn>
          <style.rewardsOptInHeader>
            {getString('rewardsOptInHeader')}
          </style.rewardsOptInHeader>
          {getString('rewardsOptInText')}
        </style.rewardsOptIn>
        <style.primaryAction>
          <AsyncButton onClick={props.onEnableRewards}>
            {getString('rewardsStartUsingRewards')}
          </AsyncButton>
        </style.primaryAction>
        <style.terms>
          <TermsOfService text={getString('rewardsOptInTerms')} />
        </style.terms>
      </style.root>
    )
  }

  function renderAdsOptIn () {
    if (props.adsEnabled || !props.adsSupported) {
      return null
    }

    return (
      <>
        <style.adsOptIn>
          {getString('rewardsOptInText')}
        </style.adsOptIn>
        <style.primaryAction>
          <AsyncButton onClick={props.onEnableAds}>
            {getString('rewardsStartUsingRewards')}
          </AsyncButton>
        </style.primaryAction>
      </>
    )
  }

  if (!props.rewardsEnabled) {
    return renderRewardsOptIn()
  }

  return (
    <style.root>
      <RewardsCardHeader />
      {renderBalance()}
      <style.progressHeader>
        <style.progressHeaderText>
          <span className='date-range'>{renderDateRange()}</span>&nbsp;
          {getString('rewardsProgress')}
        </style.progressHeaderText>
        <style.progressHeaderBorder />
      </style.progressHeader>
      {renderAdsOptIn()}
      <style.progress>
        {
          props.adsEnabled &&
            <style.earning>
              <style.progressItemLabel>
                {getString('rewardsEarning')}
                <style.earningInfo>
                  <InfoIcon />
                  <div className='tooltip'>
                    <style.earningTooltip>
                      {
                        formatMessage(getString('rewardsEarningInfoText'), [
                          nextPaymentDateFormatter.format(props.nextPaymentDate)
                        ])
                      }
                    </style.earningTooltip>
                  </div>
                </style.earningInfo>
              </style.progressItemLabel>
              <style.progressItemAmount>
                <TokenAmount amount={props.earningsThisMonth} />
              </style.progressItemAmount>
            </style.earning>
        }
        <style.giving>
          <style.progressItemLabel>
            {getString('rewardsGiving')}
          </style.progressItemLabel>
          <style.progressItemAmount>
            <TokenAmount
              amount={props.contributionsThisMonth}
              minimumFractionDigits={1}
            />
          </style.progressItemAmount>
        </style.giving>
      </style.progress>
      <style.settings>
        <NewTabLink href={urls.settingsURL}>
          <SettingsIcon />{getString('rewardsSettings')}
        </NewTabLink>
      </style.settings>
    </style.root>
  )
}
