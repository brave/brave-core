/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export const defaultState: RewardsExtension.State = {
  publishers: {},
  parameters: {
    monthlyTipChoices: [],
    rate: 0,
    autoContributeChoices: [5, 10, 15, 20]
  },
  balanceReport: {
    ads: 0.0,
    contribute: 0.0,
    monthly: 0.0,
    grant: 0.0,
    tips: 0.0
  },
  notifications: {},
  currentNotification: undefined,
  pendingContributionTotal: 0,
  enabledAC: false,
  promotions: [],
  recurringTips: [],
  tipAmounts: {},
  balance: {
    total: 0,
    wallets: {}
  },
  initializing: true,
  scheduledCaptcha: {
    url: '',
    maxAttemptsExceeded: false
  },
  showOnboarding: false,
  adsPerHour: 1,
  autoContributeAmount: 0
}

export const load = (): RewardsExtension.State => {
  return defaultState
}
