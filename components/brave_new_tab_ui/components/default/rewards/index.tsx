/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import createWidget from '../widget/index'
import { StyledCard, StyledTitleTab } from '../widgetCard'

import { LocaleContext } from '../../../../brave_rewards/resources/shared/lib/locale_context'
import { createLocaleContextForWebUI } from '../../../../brave_rewards/resources/shared/lib/webui_locale_context'
import { getProviderPayoutStatus } from '../../../../brave_rewards/resources/shared/lib/provider_payout_status'
import { userTypeFromString } from '../../../../brave_rewards/resources/shared/lib/user_type'
import {
  optional
} from '../../../../brave_rewards/resources/shared/lib/optional'

import {
  externalWalletFromExtensionData,
  isExternalWalletProviderAllowed,
  externalWalletProviderFromString,
  isSelfCustodyProvider
} from '../../../../brave_rewards/resources/shared/lib/external_wallet'

import {
  RewardsCard,
  RewardsCardHeaderContent
} from '../../../../brave_rewards/resources/shared/components/newtab'

const locale = createLocaleContextForWebUI()

export interface RewardsProps {
  rewardsEnabled: boolean
  userType: string
  declaredCountry: string
  needsBrowserUpgradeToServeAds: boolean
  balance?: number
  externalWallet?: RewardsExtension.ExternalWallet
  externalWalletProviders?: string[]
  report?: NewTab.RewardsBalanceReport
  adsAccountStatement: NewTab.AdsAccountStatement
  parameters: NewTab.RewardsParameters
  totalContribution: number
  publishersVisitedCount: number
  selfCustodyInviteDismissed: boolean
  isTermsOfServiceUpdateRequired: boolean
  showContent: boolean
  stackPosition: number
  onShowContent: () => void
  onDismissNotification: (id: string) => void
  onSelfCustodyInviteDismissed: () => void
  onTermsOfServiceUpdateAccepted: () => void
}

export const RewardsWidget = createWidget((props: RewardsProps) => {
  if (!props.showContent) {
    return (
      <StyledTitleTab onClick={props.onShowContent}>
        <LocaleContext.Provider value={locale}>
          <RewardsCardHeaderContent />
        </LocaleContext.Provider>
      </StyledTitleTab>
    )
  }

  const adsInfo = props.adsAccountStatement || null
  const externalWallet = externalWalletFromExtensionData(props.externalWallet)

  const providerPayoutStatus = () => {
    const { payoutStatus } = props.parameters
    if (!payoutStatus) {
      return 'off'
    }
    const walletProvider = externalWallet ? externalWallet.provider : null
    return getProviderPayoutStatus(payoutStatus, walletProvider)
  }

  const showSelfCustodyInvite = () => {
    if (props.userType !== 'unconnected') {
      return false
    }
    if (props.selfCustodyInviteDismissed) {
      return false
    }
    const { walletProviderRegions } = props.parameters
    for (const name of (props.externalWalletProviders || [])) {
      const provider = externalWalletProviderFromString(name)
      if (provider && isSelfCustodyProvider(provider)) {
        const regions = (walletProviderRegions || {})[provider] || null
        if (isExternalWalletProviderAllowed(props.declaredCountry, regions)) {
          return true
        }
      }
    }
    return false
  }

  const openRewardsPanel = () => {
    chrome.braveRewards.recordNTPPanelTrigger()
    chrome.braveRewards.openRewardsPanel()
  }

  return (
    <LocaleContext.Provider value={locale}>
      <StyledCard>
        <RewardsCard
          rewardsEnabled={props.rewardsEnabled}
          userType={userTypeFromString(props.userType)}
          declaredCountry={props.declaredCountry}
          needsBrowserUpgradeToServeAds={props.needsBrowserUpgradeToServeAds}
          rewardsBalance={optional(props.balance)}
          exchangeCurrency='USD'
          exchangeRate={props.parameters.rate}
          providerPayoutStatus={providerPayoutStatus()}
          externalWallet={externalWallet}
          nextPaymentDate={adsInfo ? adsInfo.nextPaymentDate : 0}
          minEarningsThisMonth={adsInfo ? adsInfo.minEarningsThisMonth : 0}
          maxEarningsThisMonth={adsInfo ? adsInfo.maxEarningsThisMonth : 0}
          minEarningsLastMonth={adsInfo ? adsInfo.minEarningsLastMonth : 0}
          maxEarningsLastMonth={adsInfo ? adsInfo.maxEarningsLastMonth : 0}
          contributionsThisMonth={props.totalContribution}
          showSelfCustodyInvite={showSelfCustodyInvite()}
          isTermsOfServiceUpdateRequired={props.isTermsOfServiceUpdateRequired}
          publishersVisited={props.publishersVisitedCount || 0}
          onEnableRewards={openRewardsPanel}
          onSelectCountry={openRewardsPanel}
          onSelfCustodyInviteDismissed={props.onSelfCustodyInviteDismissed}
          onTermsOfServiceUpdateAccepted={props.onTermsOfServiceUpdateAccepted}
        />
      </StyledCard>
    </LocaleContext.Provider>
  )
})
