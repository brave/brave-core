/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import createWidget from '../widget/index'
import { StyledTitleTab } from '../widgetCard'

import { LocaleContext } from '../../../../brave_rewards/resources/shared/lib/locale_context'
import { createLocaleContextForWebUI } from '../../../../brave_rewards/resources/shared/lib/webui_locale_context'

import {
  RewardsCardHeaderContent
} from '../../../../brave_rewards/resources/shared/components/newtab'

import { VPNPromoWidget, VPNWidget } from '../vpn/vpn_card'
import { BraveVPNState } from "components/brave_new_tab_ui/reducers/brave_vpn";
import * as BraveVPN from "../../../api/braveVpn";

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
  braveVPNState: BraveVPNState
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

  return (
    <LocaleContext.Provider value={locale}>
      {props.braveVPNState.purchasedState === BraveVPN.PurchasedState.NOT_PURCHASED ? <VPNPromoWidget /> : <VPNWidget {...props.braveVPNState} />}
    </LocaleContext.Provider>
  )
})
