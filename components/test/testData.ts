/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export class ChromeEvent {
  listeners: Array<() => void>

  constructor () {
    this.listeners = []
  }

  emit (...args: Array<() => void>) {
    this.listeners.forEach((cb: () => void) => cb.apply(null, args))
  }

  addListener (cb: () => void) {
    this.listeners.push(cb)
  }
}

export const getMockChrome = () => {
  return {
    send: () => undefined,
    getVariableValue: () => undefined,
    runtime: {
      onMessage: new ChromeEvent(),
      onConnect: new ChromeEvent(),
      onStartup: new ChromeEvent(),
      onMessageExternal: new ChromeEvent(),
      onConnectExternal: new ChromeEvent()
    },
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
    contributionMinTime: 8,
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

export const syncInitialState: Sync.ApplicationState = {
  syncData: {
    thisDeviceName: '',
    devices: [
      {
        name: '',
        id: 0,
        lastActive: ''
      }
    ],
    isSyncConfigured: false,
    seedQRImageSource: '',
    syncWords: ''
  }
}
