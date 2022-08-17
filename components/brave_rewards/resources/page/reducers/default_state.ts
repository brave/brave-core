/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export const defaultState: Rewards.State = {
  version: 3,
  createdTimestamp: null,
  enabledAds: false,
  enabledAdsMigrated: false,
  enabledContribute: false,
  firstLoad: false,
  contributionMinTime: 8,
  contributionMinVisits: 1,
  contributionMonthly: 0,
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
    adsEnabled: false,
    adsPerHour: 0,
    adsSubdivisionTargeting: '',
    automaticallyDetectedAdsSubdivisionTargeting: '',
    shouldAllowAdsSubdivisionTargeting: true,
    adsUIEnabled: false,
    adsIsSupported: false,
    needsBrowserUpgradeToServeAds: false,
    adsNextPaymentDate: 0,
    adsReceivedThisMonth: 0,
    adsEarningsThisMonth: 0,
    adsEarningsLastMonth: 0
  },
  adsHistory: [],
  pendingContributionTotal: 0,
  promotions: [],
  inlineTip: {
    twitter: true,
    reddit: true,
    github: true
  },
  pendingContributions: [],
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
    autoContributeChoices: [],
    rate: 0,
    payoutStatus: {}
  },
  initializing: true,
  paymentId: '',
  recoveryKey: '',
  showOnboarding: false
}
