/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  optional
} from '../../../../brave_rewards/resources/shared/lib/optional'
import * as Rewards from '../lib/types'

export function defaultState (): Rewards.State {
  return {
    userType: 'unconnected',
    isUserTermsOfServiceUpdateRequired: false,
    isAcSupported: false,
    enabledContribute: false,
    contributionMinTime: 8,
    contributionMinVisits: 1,
    contributionMonthly: 0,
    reconcileStamp: 0,
    ui: {
      modalConnect: false,
      modalConnectState: '',
      modalRedirect: 'hide',
      modalRedirectProvider: '',
      modalReset: false,
      modalAdsHistory: false,
      adsSettings: false,
      autoContributeSettings: false,
      promosDismissed: {}
    },
    autoContributeList: [],
    recurringList: [],
    tipsList: [],
    adsData: {
      adsPerHour: 0,
      adsSubdivisionTargeting: '',
      automaticallyDetectedAdsSubdivisionTargeting: '',
      shouldAllowAdsSubdivisionTargeting: true,
      subdivisions: [],
      adsIsSupported: false,
      needsBrowserUpgradeToServeAds: false,
      notificationAdsEnabled: false,
      newTabAdsEnabled: false,
      newsAdsEnabled: false,
      adsNextPaymentDate: 0,
      adsReceivedThisMonth: 0,
      adTypesReceivedThisMonth: {},
      adsMinEarningsThisMonth: 0,
      adsMaxEarningsThisMonth: 0,
      adsMinEarningsLastMonth: 0,
      adsMaxEarningsLastMonth: 0
    },
    adsHistory: [],
    excludedList: [],
    externalWalletProviderList: [],
    balance: optional<number>(),
    monthlyReport: {
      month: -1,
      year: -1
    },
    monthlyReportIds: [],
    currentCountryCode: '',
    parameters: {
      autoContributeChoice: 0,
      autoContributeChoices: [],
      rate: 0,
      payoutStatus: {},
      walletProviderRegions: {
        bitflyer: { allow: [], block: [] },
        gemini: { allow: [], block: [] },
        uphold: { allow: [], block: [] }
      },
      vbatDeadline: undefined,
      vbatExpired: false
    },
    initializing: true,
    showOnboarding: null,
    isUnsupportedRegion: false
  }
}
