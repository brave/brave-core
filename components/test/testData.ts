/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export const getMockChrome = () => {
  return {
    send: () => undefined,
    getVariableValue: () => undefined
  }
}

export const welcomeInitialState: Welcome.ApplicationState = {
  welcomeData: {
    pageIndex: 0
  }
}

export const rewardsInitialState: Rewards.ApplicationState = {
  rewardsData: {
    walletCreated: false,
    walletCreateFailed: false,
    createdTimestamp: null,
    enabledMain: false,
    enabledAds: false,
    enabledContribute: false,
    firstLoad: null,
    contributionMinTime: 8000,
    contributionMinVisits: 1,
    contributionMonthly: 10,
    contributionNonVerified: true,
    contributionVideos: true,
    donationAbilityYT: true,
    donationAbilityTwitter: true
  }
}

export const adblockInitialState: AdBlock.ApplicationState = {
  adblockData: {
    stats: {
      numBlocked: 0,
      regionalAdBlockEnabled: false
    }
  }
}
