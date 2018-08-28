/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
/* global chrome */

import rewardsReducer from '../../../brave_rewards/ui/reducers/rewards_reducer'
import * as actions from '../../../brave_rewards/ui/actions/rewards_actions'
import { types } from '../../../brave_rewards/ui/constants/rewards_types'

describe('rewardsReducer', () => {
  it('should handle initial state', () => {
    const assertion = rewardsReducer(undefined, actions.createWallet())
    expect(assertion).toEqual({
      walletCreated: false,
      walletCreateFailed: false,
      createdTimestamp: null,
      enabledMain: false,
      enabledAds: false,
      enabledContribute: false,
      firstLoad: null,
      contributionMinTime: 8,
      contributionMinVisits: 1,
      contributionMonthly: 10,
      contributionNonVerified: true,
      contributionVideos: true,
      donationAbilityYT: true,
      donationAbilityTwitter: true
    })
  })

  describe('CREATE_WALLET_REQUESTED', () => {
    it('calls createWalletRequested', () => {
      // TODO: mock chrome.send and use jest.spyOn()
    })
  })

  describe('WALLET_CREATED', () => {
    it('wallet created', () => {
      const assertion = rewardsReducer(undefined, {
        type: types.WALLET_CREATED,
        payload: {
          walletCreateFailed: false,
          walletCreated: true
        }
      })
      expect(assertion).toEqual({
        walletCreated: true,
        walletCreateFailed: false,
        createdTimestamp: null,
        enabledMain: false,
        enabledAds: false,
        enabledContribute: false,
        firstLoad: null,
        contributionMinTime: 8,
        contributionMinVisits: 1,
        contributionMonthly: 10,
        contributionNonVerified: true,
        contributionVideos: true,
        donationAbilityYT: true,
        donationAbilityTwitter: true
      })
    })
  })

  describe('WALLET_CREATE_FAILED', () => {
    it('wallet failed', () => {
      const assertion = rewardsReducer(undefined, {
        type: types.WALLET_CREATE_FAILED,
        payload: {
          walletCreateFailed: true,
          walletCreated: false
        }
      })
      expect(assertion).toEqual({
        walletCreated: false,
        walletCreateFailed: true,
        createdTimestamp: null,
        enabledMain: false,
        enabledAds: false,
        enabledContribute: false,
        firstLoad: null,
        contributionMinTime: 8,
        contributionMinVisits: 1,
        contributionMonthly: 10,
        contributionNonVerified: true,
        contributionVideos: true,
        donationAbilityYT: true,
        donationAbilityTwitter: true
      })
    })
  })
})
