/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { defaultState } from '../reducers/default_state'
import { optional } from '../../shared/lib/optional'

export const reduxState: Rewards.ApplicationState = {
  rewardsData: {
    ...defaultState(),

    enabledAds: false,
    enabledAdsMigrated: false,
    enabledContribute: true,
    contributionMinTime: 8,
    contributionMinVisits: 1,
    contributionMonthly: 5,
    reconcileStamp: 0,
    ui: {
      modalConnect: false,
      modalRedirect: 'hide',
      modalReset: false,
      modalAdsHistory: false,
      adsSettings: false,
      autoContributeSettings: false,
      contributionsSettings: false,
      promosDismissed: {}
    },
    autoContributeList: [
      {
        id: '1',
        name: 'brave.com',
        url: 'https://brave.com',
        publisherKey: 'brave.com',
        percentage: 1,
        status: 2,
        excluded: false,
        provider: '',
        favIcon: '',
        weight: 1
      },
      {
        id: '2',
        name: 'brave.com',
        url: 'https://brave.com',
        publisherKey: 'brave.com',
        percentage: 1,
        status: 2,
        excluded: false,
        provider: '',
        favIcon: '',
        weight: 1
      }
    ],
    recurringList: [
      {
        id: '1',
        name: 'brave.com',
        url: 'https://brave.com',
        publisherKey: 'brave.com',
        percentage: 1,
        status: 2,
        excluded: false,
        provider: '',
        favIcon: '',
        weight: 1
      },
      {
        id: '2',
        name: 'brave.com',
        url: 'https://brave.com',
        publisherKey: 'brave.com',
        percentage: 1,
        status: 2,
        excluded: false,
        provider: '',
        favIcon: '',
        weight: 1
      }
    ],
    tipsList: [
      {
        id: '1',
        name: 'brave.com',
        url: 'https://brave.com',
        publisherKey: 'brave.com',
        percentage: 1,
        status: 2,
        excluded: false,
        provider: '',
        favIcon: '',
        tipDate: new Date().getTime() / 1000,
        weight: 1
      },
      {
        id: '2',
        name: 'reallylongdomainname.com',
        url: 'https://brave.com',
        publisherKey: 'brave.com',
        percentage: 1,
        status: 2,
        excluded: false,
        provider: '',
        favIcon: '',
        weight: 1
      }
    ],
    adsData: {
      adsEnabled: false,
      adsPerHour: 0,
      adsSubdivisionTargeting: '',
      automaticallyDetectedAdsSubdivisionTargeting: '',
      shouldAllowAdsSubdivisionTargeting: true,
      subdivisions: [],
      adsUIEnabled: true,
      adsIsSupported: true,
      needsBrowserUpgradeToServeAds: false,
      adsNextPaymentDate: 0,
      adsReceivedThisMonth: 0,
      adsMinEarningsThisMonth: 0,
      adsMaxEarningsThisMonth: 0,
      adsEarningsLastMonth: 0
    },
    adsHistory: [],
    promotions: [],
    inlineTipsEnabled: true,
    inlineTip: {
      twitter: true,
      reddit: true,
      github: true
    },
    excludedList: [],
    externalWalletProviderList: ['uphold'],
    balance: optional<number>(),
    monthlyReport: {
      month: -1,
      year: -1
    },
    monthlyReportIds: [],
    currentCountryCode: '',
    parameters: {
      autoContributeChoice: 0,
      autoContributeChoices: [1, 2, 5, 10],
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
    initializing: false,
    showOnboarding: false,
    userType: 'unconnected'
  }
}
