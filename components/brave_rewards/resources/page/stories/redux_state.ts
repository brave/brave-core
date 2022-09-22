/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export const reduxState: Rewards.ApplicationState = {
  rewardsData: {
    version: 3,
    createdTimestamp: null,
    enabledAds: false,
    enabledAdsMigrated: false,
    enabledContribute: false,
    firstLoad: false,
    contributionMinTime: 8,
    contributionMinVisits: 1,
    contributionMonthly: 5,
    contributionNonVerified: true,
    contributionVideos: true,
    donationAbilityYT: true,
    donationAbilityTwitter: true,
    reconcileStamp: 0,
    ui: {
      disconnectWalletError: false,
      emptyWallet: true,
      modalBackup: false,
      modalRedirect: 'hide',
      paymentIdCheck: true,
      walletRecoveryStatus: null,
      walletServerProblem: false,
      promosDismissed: {}
    },
    autoContributeList: [],
    recurringList: [],
    tipsList: [],
    contributeLoad: false,
    recurringLoad: false,
    tipsLoad: false,
    adsData: {
      adsEnabled: true,
      adsPerHour: 0,
      adsSubdivisionTargeting: '',
      automaticallyDetectedAdsSubdivisionTargeting: '',
      shouldAllowAdsSubdivisionTargeting: true,
      adsUIEnabled: true,
      adsIsSupported: true,
      needsBrowserUpgradeToServeAds: false,
      adsNextPaymentDate: 0,
      adsReceivedThisMonth: 0,
      adsEarningsThisMonth: 0,
      adsEarningsLastMonth: 0
    },
    adsHistory: [],
    pendingContributionTotal: 4,
    promotions: [],
    inlineTip: {
      twitter: true,
      reddit: true,
      github: true
    },
    pendingContributions: [{
      id: 123,
      publisherKey: 'brave.com',
      percentage: 1,
      status: 1,
      url: 'https://brave.com',
      name: 'Brave',
      provider: 'twitter',
      favIcon: 'brave.com',
      amount: 1,
      addedDate: '',
      type: 8,
      viewingId: '',
      expirationDate: String(Date.now() / 1000)
    }, {
      id: 123,
      publisherKey: 'brave.com',
      percentage: 1,
      status: 1,
      url: 'https://brave.com',
      name: 'Brave',
      provider: '',
      favIcon: 'brave.com',
      amount: 1,
      addedDate: '',
      type: 8,
      viewingId: '',
      expirationDate: String(Date.now() / 1000)
    }, {
      id: 123,
      publisherKey: 'brave.com',
      percentage: 1,
      status: 1,
      url: 'https://brave.com',
      name: 'Brave',
      provider: '',
      favIcon: 'brave.com',
      amount: 1,
      addedDate: '',
      type: 8,
      viewingId: '',
      expirationDate: String(Date.now() / 1000)
    }, {
      id: 123,
      publisherKey: 'brave.com',
      percentage: 1,
      status: 1,
      url: 'https://brave.com',
      name: 'Brave',
      provider: '',
      favIcon: 'brave.com',
      amount: 1,
      addedDate: '',
      type: 8,
      viewingId: '',
      expirationDate: String(Date.now() / 1000)
    }],
    excludedList: [],
    externalWalletProviderList: [],
    balance: {
      total: 0,
      wallets: {}
    },
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
      payoutStatus: {}
    },
    initializing: false,
    paymentId: '',
    recoveryKey: '',
    showOnboarding: false
  }
}
