/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { getLocale } from '../../../../common/locale'
import createWidget from '../widget/index'
import { StyledTitleTab } from '../widgetTitleTab'

import { LocaleContext } from '../../../../brave_rewards/resources/shared/lib/locale_context'
import { getProviderPayoutStatus } from '../../../../brave_rewards/resources/shared/lib/provider_payout_status'
import { WithThemeVariables } from '../../../../brave_rewards/resources/shared/components/with_theme_variables'
import { GrantInfo } from '../../../../brave_rewards/resources/shared/lib/grant_info'
import { externalWalletFromExtensionData } from '../../../../brave_rewards/resources/shared/lib/external_wallet'

import {
  RewardsCard,
  RewardsCardHeader,
  SponsoredImageTooltip
} from '../../../../brave_rewards/resources/shared/components/newtab'

export { SponsoredImageTooltip }

const locale = { getString: (key: string) => getLocale(key) }

export function RewardsContextAdapter (props: { children: React.ReactNode }) {
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
  needsBrowserUpgradeToServeAds: boolean
  balance: NewTab.RewardsBalance
  externalWallet?: RewardsExtension.ExternalWallet
  report?: NewTab.RewardsBalanceReport
  adsAccountStatement: NewTab.AdsAccountStatement
  parameters: NewTab.RewardsParameters
  promotions?: NewTab.Promotion[]
  totalContribution: number
  adsSupported?: boolean
  showContent: boolean
  stackPosition: number
  onShowContent: () => void
  onEnableRewards: () => void
  onEnableAds: () => void
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
    claimableUntil: promo.claimableUntil ? promo.claimableUntil * 1000 : null,
    expiresAt: promo.expiresAt ? promo.expiresAt * 1000 : null
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
  const grantInfo = getVisibleGrant(props.promotions || [])

  const externalWallet = externalWalletFromExtensionData(props.externalWallet)
  const walletProvider = externalWallet ? externalWallet.provider : null
  const providerPayoutStatus = props.parameters.payoutStatus
    ? getProviderPayoutStatus(props.parameters.payoutStatus, walletProvider)
    : 'off'

  const onClaimGrant = () => {
    if (grantInfo) {
      chrome.braveRewards.showGrantCaptcha(grantInfo.id)
    }
  }

  return (
    <RewardsCard
      rewardsEnabled={props.rewardsEnabled}
      adsEnabled={props.enabledAds}
      adsSupported={Boolean(props.adsSupported)}
      needsBrowserUpgradeToServeAds={props.needsBrowserUpgradeToServeAds}
      rewardsBalance={props.balance.total}
      exchangeCurrency='USD'
      exchangeRate={props.parameters.rate}
      providerPayoutStatus={providerPayoutStatus}
      grantInfo={grantInfo}
      externalWallet={externalWallet}
      nextPaymentDate={adsInfo ? adsInfo.nextPaymentDate : 0}
      earningsThisMonth={adsInfo ? adsInfo.earningsThisMonth : 0}
      earningsLastMonth={adsInfo ? adsInfo.earningsLastMonth : 0}
      contributionsThisMonth={props.totalContribution}
      onEnableRewards={props.onEnableRewards}
      onEnableAds={props.onEnableAds}
      onClaimGrant={onClaimGrant}
    />
  )
})
