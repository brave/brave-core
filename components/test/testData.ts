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

export const welcomeInitialState = {
  welcomeData: {
    pageIndex: 0
  }
}

export const rewardsInitialState = {
  rewardsData: {
    walletCreated: false,
    walletCreateFailed: false
  }
}

export const adblockInitialState = {
  adblockData: {
    stats: {
      numBlocked: 0,
      regionalAdBlockEnabled: false
    }
  }
}
