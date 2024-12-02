/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import { LocaleContext, formatMessage } from '../../lib/locale_context'
import { ExternalWallet, getExternalWalletProviderName } from '../../lib/external_wallet'
import { UserType } from '../../lib/user_type'
import { ProviderPayoutStatus } from '../../lib/provider_payout_status'
import { ArrowCircleIcon } from '../icons/arrow_circle_icon'
import { BatIcon } from '../icons/bat_icon'
import { OptInIcon } from '../icons/optin_icon'
import { InfoIcon } from './icons/info_icon'
import { CaretIcon } from '../icons/caret_icon'
import { TokenAmount } from '../token_amount'
import { ExchangeAmount } from '../exchange_amount'
import { NewTabLink } from '../new_tab_link'
import { SelectCountryCard } from './select_country_card'
import { PaymentStatusView } from '../payment_status_view'
import { TosUpdateNotice } from '../tos_update_notice'
import { LoadingIcon } from '../../../shared/components/icons/loading_icon'
import { Optional } from '../../../shared/lib/optional'

import * as urls from '../../lib/rewards_urls'
import * as style from './rewards_card.style'

const monthFormatter = new Intl.DateTimeFormat(undefined, {
  month: 'short'
})

export function RewardsCardHeaderContent () {
  const { getString } = React.useContext(LocaleContext)
  return (
    <>
      <style.cardHeaderIcon>
        <BatIcon />
      </style.cardHeaderIcon>
      <div>
        {getString('rewardsBraveRewards')}
      </div>
    </>
  )
}

function RewardsCardHeader () {
  return (
    <style.cardHeader>
      <RewardsCardHeaderContent />
    </style.cardHeader>
  )
}

interface Props {
  rewardsEnabled: boolean
  userType: UserType
  declaredCountry: string
  needsBrowserUpgradeToServeAds: boolean
  rewardsBalance: Optional<number>
  exchangeRate: number
  exchangeCurrency: string
  providerPayoutStatus: ProviderPayoutStatus
  nextPaymentDate: number
  adsReceivedThisMonth: number
  minEarningsThisMonth: number
  maxEarningsThisMonth: number
  minEarningsLastMonth: number
  maxEarningsLastMonth: number
  contributionsThisMonth: number
  externalWallet: ExternalWallet | null
  publishersVisited: number
  showSelfCustodyInvite: boolean
  isTermsOfServiceUpdateRequired: boolean
  onEnableRewards: () => void
  onSelectCountry: () => void
  onSelfCustodyInviteDismissed: () => void
  onTermsOfServiceUpdateAccepted: () => void
}

export function RewardsCard (props: Props) {
  const { getString, getPluralString } = React.useContext(LocaleContext)

  const [publisherCountText, setPublisherCountText] = React.useState('')

  React.useEffect(() => {
    let active = true
    getPluralString('rewardsPublisherCountText', props.publishersVisited)
      .then((value) => { active && setPublisherCountText(value) })
    return () => { active = false }
  }, [props.publishersVisited])

  function onConnect () {
    window.open(urls.connectURL, '_blank', 'noreferrer')
  }

  function renderBalance () {
    const { externalWallet } = props
    if (externalWallet && !externalWallet.authenticated) {
      const onClick = () => {
        window.open(urls.reconnectURL, '_blank', 'noreferrer')
      }
      return (
        <style.disconnected onClick={onClick}>
          {
            formatMessage(getString('rewardsLogInToSeeBalance'), {
              placeholders: {
                $2: getExternalWalletProviderName(externalWallet.provider)
              },
              tags: {
                $1: (content) => <strong key='1'>{content}</strong>
              }
            })
          }
          <style.disconnectedArrow>
            <ArrowCircleIcon />
          </style.disconnectedArrow>
        </style.disconnected>
      )
    }

    if (props.needsBrowserUpgradeToServeAds) {
      return (
        <style.balance>
          <style.needsBrowserUpdateView>
            <div>
              {getString('rewardsBrowserCannotReceiveAds')}
            </div>
            <style.needsBrowserUpdateContentBody>
              {getString('rewardsBrowserNeedsUpdateToSeeAds')}
            </style.needsBrowserUpdateContentBody>
          </style.needsBrowserUpdateView>
          <style.pendingRewards>
          {
            props.userType === 'connected' &&
              <PaymentStatusView
                minEarnings={props.minEarningsLastMonth}
                maxEarnings={props.maxEarningsLastMonth}
                nextPaymentDate={props.nextPaymentDate}
                providerPayoutStatus={props.providerPayoutStatus}
              />
          }
          </style.pendingRewards>
        </style.balance>
      )
    }

    return (
      <style.balance
        className={!props.rewardsBalance.hasValue() ? 'flat' : ''}>
        <style.balanceTitle>
          {getString('rewardsBalanceTitle')}
        </style.balanceTitle>
        {
          !props.rewardsBalance.hasValue()
            ? <style.balanceSpinner>
                <LoadingIcon />
                <style.loading>{getString('loading')}</style.loading>
              </style.balanceSpinner>
            : <>
                <style.balanceAmount>
                  <TokenAmount amount={props.rewardsBalance.value()} />
                </style.balanceAmount>
                <style.balanceExchange>
                  ≈&nbsp;
                  <ExchangeAmount
                    amount={props.rewardsBalance.value()}
                    rate={props.exchangeRate}
                    currency={props.exchangeCurrency}
                  />
                </style.balanceExchange>
              </>
        }
        <style.pendingRewards>
        {
          props.userType === 'connected' &&
            <PaymentStatusView
              minEarnings={props.minEarningsLastMonth}
              maxEarnings={props.maxEarningsLastMonth}
              nextPaymentDate={props.nextPaymentDate}
              providerPayoutStatus={props.providerPayoutStatus}
            />
        }
        </style.pendingRewards>
      </style.balance>
    )
  }

  function renderRewardsOptIn () {
    return (
      <style.root>
        <RewardsCardHeader />
        <style.optInIcon>
          <OptInIcon />
        </style.optInIcon>
        <style.optInHeaderText>
          {getString('rewardsOptInHeader')}
        </style.optInHeaderText>
        <style.optInText>
          {getString('rewardsOptInText')}
        </style.optInText>
        <style.optInAction>
          <Button onClick={props.onEnableRewards}>
            {getString('rewardsLearnMore')}
          </Button>
        </style.optInAction>
      </style.root>
    )
  }

  function renderCountrySelect () {
    return (
      <style.root>
        <RewardsCardHeader />
        <style.selectCountry>
          <SelectCountryCard onContinue={props.onSelectCountry} />
        </style.selectCountry>
      </style.root>
    )
  }

  function renderTosUpdateNotice () {
    const onReset = () => {
      window.open(urls.resetURL, '_blank', 'noreferrer')
    }

    return (
      <style.root>
        <RewardsCardHeader />
        <style.tosUpdateNotice>
          <TosUpdateNotice
            onAccept={props.onTermsOfServiceUpdateAccepted}
            onResetRewards={onReset}
          />
        </style.tosUpdateNotice>
      </style.root>
    )
  }

  function renderEarnings () {
    return (
      <style.earnings>
        <style.earningsHeader>
          <style.earningsHeaderText>
            {getString('rewardsEarningsTitle')}
            <style.earningsInfo>
              <InfoIcon />
              <div className='tooltip'>
                <style.earningsTooltip>
                  {getString('rewardsEarningInfoText')}
                  <style.manageAds>
                    <NewTabLink href={urls.settingsURL}>
                      {getString('rewardsManageAds')}
                      <CaretIcon direction='right' />
                    </NewTabLink>
                  </style.manageAds>
                </style.earningsTooltip>
              </div>
            </style.earningsInfo>
          </style.earningsHeaderText>
        </style.earningsHeader>
        <style.earningsDisplay>
          <style.earningsMonth>
            {monthFormatter.format(new Date())}
          </style.earningsMonth>
          <div>
            {props.adsReceivedThisMonth}
          </div>
        </style.earningsDisplay>
      </style.earnings>
    )
  }

  function renderSelfCustodyInvite () {
    const onConnectSelfCustody = () => {
      props.onSelfCustodyInviteDismissed()
      onConnect()
    }
    return (
      <style.root>
        <RewardsCardHeader />
        <style.selfCustodyInvite>
          <style.selfCustodyInviteHeader>
            <style.selfCustodyInviteClose>
              <button onClick={props.onSelfCustodyInviteDismissed}>
                <Icon name='close' />
              </button>
            </style.selfCustodyInviteClose>
            {getString('rewardsSelfCustodyInviteHeader')}
          </style.selfCustodyInviteHeader>
          <style.selfCustodyInviteText>
            {getString('rewardsSelfCustodyInviteText')}
          </style.selfCustodyInviteText>
          <style.connectAction>
            <Button onClick={onConnectSelfCustody}>
              {getString('rewardsConnectAccount')}
            </Button>
          </style.connectAction>
          <style.selfCustodyInviteDismiss>
            <button onClick={props.onSelfCustodyInviteDismissed}>
              {getString('rewardsNotNow')}
            </button>
          </style.selfCustodyInviteDismiss>
        </style.selfCustodyInvite>
      </style.root>
    )
  }

  function renderLimited () {
    return (
      <style.root>
        <RewardsCardHeader />
        <style.connect>
          {
            formatMessage(getString('rewardsConnectAccountText'), {
              tags: {
                $1: (content) => <strong key='bold'>{content}</strong>
              }
            })
          }
          <style.connectAction>
            <Button onClick={onConnect}>
              {getString('rewardsConnectAccount')}
            </Button>
          </style.connectAction>
        </style.connect>
        <style.publisherSupport>
          <style.publisherCount>
            {props.publishersVisited}
          </style.publisherCount>
          <div>{publisherCountText}</div>
        </style.publisherSupport>
      </style.root>
    )
  }

  if (!props.rewardsEnabled) {
    return renderRewardsOptIn()
  }

  if (!props.declaredCountry) {
    return renderCountrySelect()
  }

  if (props.isTermsOfServiceUpdateRequired) {
    return renderTosUpdateNotice()
  }

  if (props.showSelfCustodyInvite) {
    return renderSelfCustodyInvite()
  }

  if (props.userType === 'unconnected') {
    return renderLimited()
  }

  return (
    <style.root>
      <RewardsCardHeader />
      {renderBalance()}
      {renderEarnings()}
    </style.root>
  )
}
