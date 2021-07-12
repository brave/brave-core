/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Utils
import { debounce } from '../../../common/debounce'

const keyName = 'rewards-data'

export const defaultState: Rewards.State = {
  createdTimestamp: null,
  enabledAds: true,
  enabledAdsMigrated: false,
  enabledContribute: true,
  firstLoad: null,
  contributionMinTime: 8,
  contributionMinVisits: 1,
  contributionMonthly: 10,
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
    verifyOnboardingDisplayed: false
  },
  autoContributeList: [],
  safetyNetFailed: false,
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
    adsEstimatedPendingRewards: 0,
    adsNextPaymentDate: '',
    adsReceivedThisMonth: 0
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
  recoveryKey: ''
}

const cleanData = (state: Rewards.State) => {
  if (!state.balance) {
    state.balance = defaultState.balance
  }

  if (!state.parameters) {
    state.parameters = defaultState.parameters
  }

  return state
}

export const load = (): Rewards.State => {
  const data = window.localStorage.getItem(keyName)
  let state: Rewards.State = defaultState
  if (data) {
    try {
      state = JSON.parse(data)
      state.initializing = true
    } catch (e) {
      console.error('Could not parse local storage data: ', e)
    }
  }
  return cleanData(state)
}

export const debouncedSave = debounce((data: Rewards.State) => {
  if (data) {
    window.localStorage.setItem(keyName, JSON.stringify(cleanData(data)))
  }
}, 50)
