/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { getLocale } from '../../../../common/locale'
import createWidget from '../widget/index'
import { StyledTitleTab } from '../widgetTitleTab'

import { LocaleContext } from '../../../../brave_rewards/resources/shared/lib/locale_context'
import { WithThemeVariables } from '../../../../brave_rewards/resources/shared/components/with_theme_variables'
import { GrantInfo } from '../../../../brave_rewards/resources/shared/lib/grant_info'

import {
  RewardsCard,
  RewardsCardHeader,
  SponsoredImageTooltip
} from '../../../../brave_rewards/resources/shared/components/newtab'

export { SponsoredImageTooltip }

const locale = { getString: (key: string) => getLocale(key) }

export function RewardsContextAdapter (props: { children: React.ReactNode }) {
  console.log('Rerendering RewardsContextAdapter')
  return (
    <LocaleContext.Provider value={locale}>
      <WithThemeVariables>
        {props.children}
      </WithThemeVariables>
    </LocaleContext.Provider>
  )
}

export interface RewardsProps {
  rewardsEnabled: boolean
  enabledAds: boolean
  balance: NewTab.RewardsBalance
  adsAccountStatement: NewTab.AdsAccountStatement
  parameters: NewTab.RewardsParameters
  promotions: NewTab.Promotion[]
  totalContribution: number
  adsSupported?: boolean
  showContent: boolean
  stackPosition: number
  onShowContent: () => void
  onStartRewards: () => void
  onDismissNotification: (id: string) => void
}

function getVisibleGrant (promotions: NewTab.Promotion[]): GrantInfo | null {
  if (promotions.length === 0) {
    return null
  }
  const promo = promotions[0]
  return {
    id: promo.promotionId,
    amount: promo.amount,
    type: promo.type === 1 ? 'ads' : 'ugp',
    createdAt: promo.createdAt * 1000,
    expiresAt: null
  }
}

export const RewardsWidget = createWidget((props: RewardsProps) => {
  if (!props.showContent) {
    return (
      <StyledTitleTab
        onClick={props.onShowContent}
        stackPosition={props.stackPosition}
      >
        <RewardsCardHeader />
      </StyledTitleTab>
    )
  }

  const adsInfo = props.adsAccountStatement || null
  const grantInfo = getVisibleGrant(props.promotions)

  const onDismissGrant = () => {
    if (grantInfo) {
      props.onDismissNotification(grantInfo.id)
    }
  }

  const onClaimGrant = () => {
    onDismissGrant()
    if (grantInfo) {
      chrome.braveRewards.openBrowserActionUI(
        `brave_rewards_panel.html#grant_${grantInfo.id}`)
    }
  }

  return (
    <RewardsCard
      rewardsEnabled={props.rewardsEnabled}
      adsEnabled={props.enabledAds}
      adsSupported={Boolean(props.adsSupported)}
      rewardsBalance={props.balance.total}
      exchangeCurrency='USD'
      exchangeRate={props.parameters.rate}
      grantInfo={grantInfo}
      nextPaymentDate={adsInfo ? adsInfo.nextPaymentDate : 0}
      earningsThisMonth={adsInfo ? adsInfo.earningsThisMonth : 0}
      earningsLastMonth={adsInfo ? adsInfo.earningsLastMonth : 0}
      contributionsThisMonth={props.totalContribution}
      onEnableRewards={props.onStartRewards}
      onEnableAds={props.onStartRewards}
      onDismissGrant={onDismissGrant}
      onClaimGrant={onClaimGrant}
    />
  )
})
