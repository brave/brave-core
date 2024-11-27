/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { HostState } from './interfaces'
import { optional } from '../../shared/lib/optional'

export function getInitialState (): HostState {
  return {
    openTime: Date.now(),
    loading: true,
    requestedView: null,
    rewardsEnabled: false,
    balance: optional<number>(),
    options: {
      externalWalletRegions: new Map(),
      vbatDeadline: undefined,
      vbatExpired: false
    },
    adaptiveCaptchaInfo: null,
    exchangeInfo: {
      currency: 'USD',
      rate: 0
    },
    earningsInfo: {
      nextPaymentDate: 0,
      adsReceivedThisMonth: 0,
      minEarningsThisMonth: 0,
      maxEarningsThisMonth: 0,
      minEarningsLastMonth: 0,
      maxEarningsLastMonth: 0
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
      monthlyTips: 0
    },
    notifications: [],
    availableCountries: [],
    defaultCountry: '',
    declaredCountry: '',
    userType: 'unconnected',
    publishersVisitedCount: 0,
    selfCustodyInviteDismissed: false,
    isTermsOfServiceUpdateRequired: false
  }
}
