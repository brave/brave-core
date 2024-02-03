/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import Icon from '@brave/leo/react/icon'

import { LocaleContext, formatMessage } from '../../lib/locale_context'
import { GrantInfo } from '../../lib/grant_info'
import { ExternalWallet, getExternalWalletProviderName } from '../../lib/external_wallet'
import { UserType } from '../../lib/user_type'
import { ProviderPayoutStatus } from '../../lib/provider_payout_status'
import { ArrowCircleIcon } from '../icons/arrow_circle_icon'
import { BatIcon } from '../icons/bat_icon'
import { OptInIcon } from '../icons/optin_icon'
import { SettingsIcon } from '../icons/settings_icon'
import { InfoIcon } from './icons/info_icon'
import { ArrowNextIcon } from '../icons/arrow_next_icon'
import { CaretIcon } from '../icons/caret_icon'
import { EarningsRange } from '../earnings_range'
import { TokenAmount } from '../token_amount'
import { ExchangeAmount } from '../exchange_amount'
import { NewTabLink } from '../new_tab_link'
import { TermsOfService } from '../terms_of_service'
import { GrantOverlay } from './grant_overlay'
import { SelectCountryCard } from './select_country_card'
import { PaymentStatusView } from '../payment_status_view'
import { UnsupportedRegionCard } from './unsupported_region_card'
import { VBATNotice, shouldShowVBATNotice } from '../vbat_notice'
import { LoadingIcon } from '../../../shared/components/icons/loading_icon'
import { Optional } from '../../../shared/lib/optional'

import * as urls from '../../lib/rewards_urls'

import * as style from './rewards_card.style'

import * as mojom from '../../../shared/lib/mojom'

const monthFormatter = new Intl.DateTimeFormat(undefined, {
  month: 'short'
})

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
  userType: UserType
  vbatDeadline: number | undefined
  isUnsupportedRegion: boolean
  declaredCountry: string
  needsBrowserUpgradeToServeAds: boolean
  rewardsBalance: Optional<number>
  exchangeRate: number
  exchangeCurrency: string
  providerPayoutStatus: ProviderPayoutStatus
  nextPaymentDate: number
  minEarningsThisMonth: number
  maxEarningsThisMonth: number
  minEarningsLastMonth: number
  maxEarningsLastMonth: number
  contributionsThisMonth: number
  grantInfo: GrantInfo | null
  externalWallet: ExternalWallet | null
  publishersVisited: number
  canConnectAccount: boolean
  showSelfCustodyInvite: boolean
  onEnableRewards: () => void
  onSelectCountry: () => void
  onClaimGrant: () => void
  onSelfCustodyInviteDismissed: () => void
}

export function RewardsCard (props: Props) {
  const { getString, getPluralString } = React.useContext(LocaleContext)

  const [publisherCountText, setPublisherCountText] = React.useState('')
  const [hideVBATNotice, setHideVBATNotice] = React.useState(false)

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
    if (props.grantInfo && props.grantInfo.amount > 0) {
      return (
        <GrantOverlay
          grantInfo={props.grantInfo}
          onClaim={props.onClaimGrant}
        />
      )
    }

    const { externalWallet } = props
    if (externalWallet && externalWallet.status === mojom.WalletStatus.kLoggedOut) {
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
            <style.needsBrowserUpdateContentHeader>
              {getString('rewardsBrowserCannotReceiveAds')}
            </style.needsBrowserUpdateContentHeader>
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
                  <style.balanceExchangeAmount>
                    ≈&nbsp;
                    <ExchangeAmount
                      amount={props.rewardsBalance.value()}
                      rate={props.exchangeRate}
                      currency={props.exchangeCurrency} />
                  </style.balanceExchangeAmount>
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

  function renderRewardsUnsupportedRegion () {
    return (
      <style.root>
        <RewardsCardHeader />
        <style.unsupportedRegionCard>
          <UnsupportedRegionCard />
        </style.unsupportedRegionCard>
      </style.root>
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
          <button onClick={props.onEnableRewards}>
            {getString('rewardsStartUsingRewards')}
          </button>
        </style.optInAction>
        <style.optInLearnMore>
          <NewTabLink href={urls.rewardsTourURL}>
            {getString('rewardsHowDoesItWork')}
          </NewTabLink>
        </style.optInLearnMore>
        <style.optInTerms>
          <TermsOfService text={getString('rewardsOptInTerms')} />
        </style.optInTerms>
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

  function renderEarnings () {
    return (
      <>
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
          <style.earningsHeaderBorder />
        </style.earningsHeader>
        <style.earningsDisplay>
          <style.earningsMonth>
            {monthFormatter.format(new Date())}
          </style.earningsMonth>
          <div>
            {
              props.userType === 'connected'
                ? <EarningsRange
                    minimum={props.minEarningsThisMonth}
                    maximum={props.maxEarningsThisMonth}
                    minimumFractionDigits={3}
                  />
                : <style.hiddenEarnings>
                    --&nbsp;
                    <NewTabLink href={urls.rewardsChangesURL}>
                      {getString('rewardsLearnMore')}
                    </NewTabLink>
                  </style.hiddenEarnings>
            }
          </div>
        </style.earningsDisplay>
      </>
    )
  }

  function renderSettingsLink () {
    return (
      <style.settings>
        <NewTabLink href={urls.settingsURL}>
          <SettingsIcon />{getString('rewardsSettings')}
        </NewTabLink>
      </style.settings>
    )
  }

  function renderVBATNotice () {
    const onClose = () => { setHideVBATNotice(true) }
    return (
      <style.root>
        <RewardsCardHeader />
        <style.vbatNotice>
          <VBATNotice
            vbatDeadline={props.vbatDeadline}
            canConnectAccount={props.canConnectAccount}
            declaredCountry={props.declaredCountry}
            onConnectAccount={onConnect}
            onClose={onClose}
          />
        </style.vbatNotice>
      </style.root>
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
            <button onClick={onConnectSelfCustody}>
              {getString('rewardsConnectAccount')}<ArrowNextIcon />
            </button>
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
            props.canConnectAccount
              ? <>
                  {
                    formatMessage(getString('rewardsConnectAccountText'), {
                      tags: {
                        $1: (content) => <strong key='bold'>{content}</strong>
                      }
                    })
                  }
                  <style.connectAction>
                    <button onClick={onConnect}>
                      {getString('rewardsConnectAccount')}<ArrowNextIcon />
                    </button>
                  </style.connectAction>
                </>
              : <>
                  {getString('rewardsConnectAccountNoProviders')}
                  <style.connectLearnMore>
                    <NewTabLink href={urls.supportedWalletRegionsURL}>
                      {getString('rewardsLearnMore')}
                    </NewTabLink>
                  </style.connectLearnMore>
                </>
          }
        </style.connect>
        {
          <style.publisherSupport>
            <style.publisherCount>
              {props.publishersVisited}
            </style.publisherCount>
            <div>{publisherCountText}</div>
          </style.publisherSupport>
        }
        {renderSettingsLink()}
      </style.root>
    )
  }

  if (props.isUnsupportedRegion) {
    return renderRewardsUnsupportedRegion()
  }

  if (!props.rewardsEnabled) {
    return renderRewardsOptIn()
  }

  if (!props.declaredCountry) {
    return renderCountrySelect()
  }

  if (!hideVBATNotice) {
    if (shouldShowVBATNotice(props.userType, props.vbatDeadline)) {
      return renderVBATNotice()
    }
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
      {renderSettingsLink()}
    </style.root>
  )
}
