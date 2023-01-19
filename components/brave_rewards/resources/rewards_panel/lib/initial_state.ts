/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { HostState } from './interfaces'

export function getInitialState (): HostState {
  return {
    openTime: Date.now(),
    loading: true,
    requestedView: null,
    rewardsEnabled: false,
    balance: 0,
    settings: {
      adsEnabled: false,
      adsPerHour: 0,
      autoContributeEnabled: false,
      autoContributeAmount: 0
    },
    options: {
      autoContributeAmounts: [],
      externalWalletRegions: new Map(),
      vbatDeadline: undefined,
      vbatExpired: false
    },
    grantCaptchaInfo: null,
    adaptiveCaptchaInfo: null,
    exchangeInfo: {
      currency: 'USD',
      rate: 0
    },
    earningsInfo: {
      nextPaymentDate: 0,
      earningsThisMonth: 0,
      earningsLastMonth: 0
    },
    payoutStatus: {},
    publisherInfo: null,
    publisherRefreshing: false,
    externalWalletProviders: [],
    externalWallet: null,
    summaryData: {
      adEarnings: 0,
      autoContributions: 0,
      oneTimeTips: 0,
      monthlyTips: 0,
      pendingTips: 0
    },
    notifications: [],
    availableCountries: [],
    declaredCountry: '',
    userType: 'unconnected',
    publishersVisitedCount: 0
  }
}
