/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Utils
import { debounce } from '../../../common/debounce'

const keyName = 'rewards-data'

export const defaultState: Rewards.State = {
  version: 1,
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
    rate: 0
  },
  initializing: true,
  paymentId: '',
  recoveryKey: '',
  showOnboarding: false
}

export const load = (): Rewards.State => {
  const data = window.localStorage.getItem(keyName)
  if (!data) {
    return defaultState
  }

  let parsedData: any
  try {
    parsedData = JSON.parse(data)
  } catch {
    parsedData = null
  }

  if (!parsedData || typeof parsedData !== 'object') {
    console.error('Local storage data is not an object')
    return defaultState
  }

  if (parsedData.version !== defaultState.version) {
    console.error('Local storage state version does not match')
    return defaultState
  }

  parsedData.initializing = true
  return parsedData as Rewards.State
}

export const debouncedSave = debounce((data: Rewards.State) => {
  if (data) {
    window.localStorage.setItem(keyName, JSON.stringify(data))
  }
}, 150)

export const save = (data: Rewards.State) => {
  window.localStorage.setItem(keyName, JSON.stringify(data))
}
