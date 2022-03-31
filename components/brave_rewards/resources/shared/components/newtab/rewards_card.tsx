/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext, formatMessage } from '../../lib/locale_context'
import { GrantInfo } from '../../lib/grant_info'
import { BatIcon } from '../icons/bat_icon'
import { SettingsIcon } from '../icons/settings_icon'
import { InfoIcon } from './icons/info_icon'
import { TermsOfService } from '../terms_of_service'
import { AsyncButton } from './async_button'
import { TokenAmount } from '../token_amount'
import { ExchangeAmount } from '../exchange_amount'
import { NewTabLink } from '../new_tab_link'
import { GrantOverlay } from './grant_overlay'
import { PaymentStatusView, shouldRenderPendingRewards } from '../payment_status_view'

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

  // To get the last day of the month, we can create a date for the next month,
  // (months greater than 11 wrap around) with the day part set to 0 (one before
  // the first day of the month). The Date constructor will perform the offset
  // for us.
  const end = new Date(start.getFullYear(), start.getMonth() + 1, 0)

  return (
    <>{monthDayFormatter.format(start)} - {monthDayFormatter.format(end)}</>
  )
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
  earningsReceived: boolean
  contributionsThisMonth: number
  grantInfo: GrantInfo | null
  onEnableRewards: () => void
  onEnableAds: () => void
  onClaimGrant: () => void
}

export function RewardsCard (props: Props) {
  const { getString } = React.useContext(LocaleContext)

  function renderBalance () {
    if (props.grantInfo && props.grantInfo.amount > 0) {
      return (
        <GrantOverlay
          grantInfo={props.grantInfo}
          onClaim={props.onClaimGrant}
        />
      )
    }

    const showPending = shouldRenderPendingRewards(
      props.earningsLastMonth,
      props.nextPaymentDate)

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
            props.rewardsBalance > 0 && !showPending &&
              <style.balanceExchangeNote>
                {getString('rewardsExchangeValueNote')}
              </style.balanceExchangeNote>
          }
        </style.balanceExchange>
        <style.pendingRewards>
          <PaymentStatusView
            earningsLastMonth={props.earningsLastMonth}
            earningsReceived={props.earningsReceived}
            nextPaymentDate={props.nextPaymentDate}
          />
        </style.pendingRewards>
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
