/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Utils
import { debounce } from '../../../common/debounce'

const keyName = 'rewards-data'

export const defaultState: Rewards.State = {
  createdTimestamp: null,
  enabledMain: false,
  enabledAds: true,
  enabledContribute: true,
  firstLoad: null,
  walletCreated: false,
  walletCreateFailed: false,
  contributionMinTime: 8,
  contributionMinVisits: 1,
  contributionMonthly: 10,
  contributionNonVerified: true,
  contributionVideos: true,
  donationAbilityYT: true,
  donationAbilityTwitter: true,
  excludedPublishersNumber: 0,
  walletInfo: {
    balance: 0,
    choices: [5.0, 7.5, 10.0, 17.5, 25.0, 50.0, 75.0, 100.0],
    probi: '0'
  },
  connectedWallet: false,
  recoveryKey: '',
  reconcileStamp: 0,
  ui: {
    addressCheck: true,
    emptyWallet: true,
    modalBackup: false,
    paymentIdCheck: true,
    walletCorrupted: false,
    walletImported: false,
    walletRecoverySuccess: null,
    walletServerProblem: false
  },
  autoContributeList: [],
  reports: {},
  recurringList: [],
  tipsList: [],
  contributeLoad: false,
  recurringLoad: false,
  tipsLoad: false,
  adsData: {
    adsEnabled: false,
    adsPerHour: 0,
    adsUIEnabled: false,
    adsNotificationsReceived: 0,
    adsEstimatedEarnings: 0,
    adsIsSupported: false
  },
  pendingContributionTotal: 0,
  grants: [],
  currentGrant: undefined,
  inlineTip: {
    twitter: true
  }
}

const cleanData = (state: Rewards.State) => state

export const load = (): Rewards.State => {
  const data = window.localStorage.getItem(keyName)
  let state: Rewards.State = defaultState
  if (data) {
    try {
      state = JSON.parse(data)
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
