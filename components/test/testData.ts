/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { defaultState as welcomeData } from '../../components/brave_welcome_ui/storage'
import { defaultState as rewardsData } from '../../components/brave_rewards/resources/ui/storage'
import { defaultState as adblockData } from '../../components/brave_adblock_ui/storage'
import { defaultState as syncData } from '../../components/brave_sync/ui/storage'

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
    braveRewards: {
      getPublisherData: (id: number, url: string, favicon: string) => undefined
    },
    extension: {
      inIncognitoContext: new ChromeEvent()
    },
    topSites: {
      get: function () {
        return
      }
    },
    bookmarks: {
      search: function () {
        return
      }
    }
  }
}

export const welcomeInitialState: Welcome.ApplicationState = { welcomeData }

export const rewardsInitialState: Rewards.ApplicationState = { rewardsData }

export const adblockInitialState: AdBlock.ApplicationState = { adblockData }

export const syncInitialState: Sync.ApplicationState = { syncData }
