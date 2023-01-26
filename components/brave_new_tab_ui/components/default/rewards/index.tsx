/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import createWidget from '../widget/index'
import { StyledTitleTab } from '../widgetTitleTab'

import { LocaleContext } from '../../../../brave_rewards/resources/shared/lib/locale_context'
import { createLocaleContextForWebUI } from '../../../../brave_rewards/resources/shared/lib/webui_locale_context'
import { getProviderPayoutStatus } from '../../../../brave_rewards/resources/shared/lib/provider_payout_status'
import { WithThemeVariables } from '../../../../brave_rewards/resources/shared/components/with_theme_variables'
import { GrantInfo } from '../../../../brave_rewards/resources/shared/lib/grant_info'
import { userTypeFromString } from '../../../../brave_rewards/resources/shared/lib/user_type'

import {
  externalWalletFromExtensionData,
  isExternalWalletProviderAllowed
} from '../../../../brave_rewards/resources/shared/lib/external_wallet'

import {
  RewardsCard,
  RewardsCardHeader
} from '../../../../brave_rewards/resources/shared/components/newtab'

const locale = createLocaleContextForWebUI()

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
  userType: string
  isUnsupportedRegion: boolean
  declaredCountry: string
  enabledAds: boolean
  needsBrowserUpgradeToServeAds: boolean
  balance: NewTab.RewardsBalance
  externalWallet?: RewardsExtension.ExternalWallet
  externalWalletProviders?: string[]
  report?: NewTab.RewardsBalanceReport
  adsAccountStatement: NewTab.AdsAccountStatement
  parameters: NewTab.RewardsParameters
  promotions?: NewTab.Promotion[]
  totalContribution: number
  publishersVisitedCount: number
  adsSupported?: boolean
  showContent: boolean
  stackPosition: number
  onShowContent: () => void
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

  const providerPayoutStatus = () => {
    const { payoutStatus } = props.parameters
    if (!payoutStatus) {
      return 'off'
    }
    const walletProvider = externalWallet ? externalWallet.provider : null
    return getProviderPayoutStatus(payoutStatus, walletProvider)
  }

  const canConnectAccount = () => {
    const providers = props.externalWalletProviders || []
    const { walletProviderRegions } = props.parameters
    if (providers.length === 0 || !walletProviderRegions) {
      return true
    }
    for (const provider of providers) {
      const regions = walletProviderRegions[provider] || null
      if (isExternalWalletProviderAllowed(props.declaredCountry, regions)) {
        return true
      }
    }
    return false
  }

  const onClaimGrant = () => {
    if (grantInfo) {
      chrome.braveRewards.showGrantCaptcha(grantInfo.id)
    }
  }

  const openRewardsPanel = () => {
    chrome.braveRewards.openRewardsPanel()
  }

  const enableAds = () => {
    chrome.braveRewards.enableAds()
  }

  return (
    <RewardsCard
      rewardsEnabled={props.rewardsEnabled}
      userType={userTypeFromString(props.userType)}
      vbatDeadline={props.parameters.vbatDeadline}
      isUnsupportedRegion={props.isUnsupportedRegion}
      declaredCountry={props.declaredCountry}
      adsEnabled={props.enabledAds}
      adsSupported={Boolean(props.adsSupported)}
      needsBrowserUpgradeToServeAds={props.needsBrowserUpgradeToServeAds}
      rewardsBalance={props.balance.total}
      exchangeCurrency='USD'
      exchangeRate={props.parameters.rate}
      providerPayoutStatus={providerPayoutStatus()}
      grantInfo={grantInfo}
      externalWallet={externalWallet}
      nextPaymentDate={adsInfo ? adsInfo.nextPaymentDate : 0}
      earningsThisMonth={adsInfo ? adsInfo.earningsThisMonth : 0}
      earningsLastMonth={adsInfo ? adsInfo.earningsLastMonth : 0}
      contributionsThisMonth={props.totalContribution}
      canConnectAccount={canConnectAccount()}
      publishersVisited={props.publishersVisitedCount || 0}
      onEnableRewards={openRewardsPanel}
      onEnableAds={enableAds}
      onSelectCountry={openRewardsPanel}
      onClaimGrant={onClaimGrant}
    />
  )
})
