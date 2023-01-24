/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext, formatMessage } from '../../lib/locale_context'
import { GrantInfo } from '../../lib/grant_info'
import { ExternalWallet, getExternalWalletProviderName } from '../../lib/external_wallet'
import { UserType } from '../../lib/user_type'
import { ProviderPayoutStatus } from '../../lib/provider_payout_status'
import { ArrowCircleIcon } from '../icons/arrow_circle_icon'
import { BatIcon } from '../icons/bat_icon'
import { SettingsIcon } from '../icons/settings_icon'
import { InfoIcon } from './icons/info_icon'
import { ArrowNextIcon } from '../icons/arrow_next_icon'
import { TermsOfService } from '../terms_of_service'
import { TokenAmount } from '../token_amount'
import { ExchangeAmount } from '../exchange_amount'
import { NewTabLink } from '../new_tab_link'
import { GrantOverlay } from './grant_overlay'
import { SelectCountryCard } from './select_country_card'
import { PaymentStatusView } from '../payment_status_view'
import { UnsupportedRegionCard } from './unsupported_region_card'
import { VBATNotice, shouldShowVBATNotice } from '../vbat_notice'

import * as urls from '../../lib/rewards_urls'

import * as style from './rewards_card.style'

import * as mojom from '../../../shared/lib/mojom'

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
  userType: UserType
  vbatDeadline: number | undefined
  isUnsupportedRegion: boolean
  declaredCountry: string
  adsEnabled: boolean
  adsSupported: boolean
  needsBrowserUpgradeToServeAds: boolean
  rewardsBalance: number
  exchangeRate: number
  exchangeCurrency: string
  providerPayoutStatus: ProviderPayoutStatus
  nextPaymentDate: number
  earningsThisMonth: number
  earningsLastMonth: number
  contributionsThisMonth: number
  grantInfo: GrantInfo | null
  externalWallet: ExternalWallet | null
  publishersVisited: number
  canConnectAccount: boolean
  onEnableRewards: () => void
  onEnableAds: () => void
  onSelectCountry: () => void
  onClaimGrant: () => void
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
        if (externalWallet.links.reconnect) {
          window.open(externalWallet.links.reconnect, '_blank')
        }
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

    if (props.needsBrowserUpgradeToServeAds && props.adsSupported) {
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
            <PaymentStatusView
              earningsLastMonth={props.earningsLastMonth}
              nextPaymentDate={props.nextPaymentDate}
              providerPayoutStatus={props.providerPayoutStatus}
            />
          </style.pendingRewards>
        </style.balance>
      )
    }

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
            props.rewardsBalance > 0 &&
              <style.balanceExchangeNote>
                {getString('rewardsExchangeValueNote')}
              </style.balanceExchangeNote>
          }
        </style.balanceExchange>
        <style.pendingRewards>
          <PaymentStatusView
            earningsLastMonth={props.earningsLastMonth}
            nextPaymentDate={props.nextPaymentDate}
            providerPayoutStatus={props.providerPayoutStatus}
          />
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
        <style.rewardsOptIn>
          <style.rewardsOptInHeader>
            {getString('rewardsOptInHeader')}
          </style.rewardsOptInHeader>
          {getString('rewardsOptInText')}
        </style.rewardsOptIn>
        <style.primaryAction>
          <button onClick={props.onEnableRewards}>
            {getString('rewardsStartUsingRewards')}
          </button>
        </style.primaryAction>
        <style.terms>
          <TermsOfService text={getString('rewardsOptInTerms')} />
        </style.terms>
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
          <button onClick={props.onEnableAds}>
            {getString('rewardsEnableBraveAds')}
          </button>
        </style.primaryAction>
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
    const onConnect = () => { window.open(urls.connectURL, '_blank') }
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

  function renderLimited () {
    const onConnect = () => { window.open(urls.connectURL, '_blank') }

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
          props.publishersVisited > 0 &&
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

  if (props.userType === 'unconnected') {
    return renderLimited()
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
                {
                  props.externalWallet
                    ? <TokenAmount
                        amount={props.earningsThisMonth}
                        minimumFractionDigits={1}
                      />
                    : <style.hiddenEarnings>
                        -&nbsp;-&nbsp;
                        <NewTabLink href={urls.rewardsChangesURL}>
                          {getString('rewardsLearnMore')}
                        </NewTabLink>
                      </style.hiddenEarnings>
                }
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
      {renderSettingsLink()}
    </style.root>
  )
}
