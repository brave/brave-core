// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

export {}

type loadTimeData = {
  getString: (key: string) => string
  getInteger: (key: string) => number
  getBoolean: (key: string) => boolean
  data_: Record<string, string>
}

type RequestIdleCallbackHandle = any
type RequestIdleCallbackOptions = {
  timeout: number
}
type RequestIdleCallbackDeadline = {
  readonly didTimeout: boolean;
  timeRemaining: (() => number)
}

declare global {
  interface Window {
    // Typescript doesn't include requestIdleCallback as it's non-standard.
    // Since it's supported in Chromium, we can include it here.
    requestIdleCallback: ((
      callback: ((deadline: RequestIdleCallbackDeadline) => void),
      opts?: RequestIdleCallbackOptions
    ) => RequestIdleCallbackHandle)
    cancelIdleCallback: ((handle: RequestIdleCallbackHandle) => void)
    loadTimeData: loadTimeData
    cr: {
      define: (name: string, init: () => void) => void
      sendWithPromise<T> (method: string, ...args: any[]): Promise<T>
      addWebUIListener: (eventName: string, callback: (...args: any[]) => void) => void
    }
    i18nTemplate: {
      process: (document: Document, translations: loadTimeData) => void
    }
    brave_adblock: {
      initialize: () => void
    }
    playlist: {
      initialize: () => void
    }
    brave_new_tab: {
      initialize: () => void
      statsUpdated: () => void
    }
    brave_rewards: {
      initialize: () => void
      parameters: chrome.events.Event<(properties: Rewards.RewardsParameters) => void>
      recoverWalletData: chrome.events.Event<(result: number) => void>
      reconcileStamp: chrome.events.Event<(stamp: number) => void>
      addresses: chrome.events.Event<(addresses: Record<string, string>) => void>
      contributeList: chrome.events.Event<(list: Rewards.Publisher[]) => void>
      balanceReports: chrome.events.Event<(reports: Record<string, Rewards.BalanceReport>) => void>
    }
    brave_welcome: {
      initialize: () => void
    }
    brave_rewards_tip: {
      initialize: () => void
    }
    brave_rewards_internals: {
      initialize: () => void
    }
    ipfs: {
      initialize: () => void
    }
    sync_ui_exports: {
      initialize: () => void
    }
    tor_internals: {
      initialize: () => void
    }
    alreadyInserted: boolean
    web3: any
    youtubedown_urls: any
    content_cosmetic: {
      cosmeticStyleSheet: CSSStyleSheet
      allSelectorsToRules: Map<string, number>
      observingHasStarted: boolean
      hide1pContent: boolean
      generichide: boolean
      firstRunQueue: Set<string>
      secondRunQueue: Set<string>
      finalRunQueue: Set<string>
      allQueues: Set<string>[]
      numQueues: any
      alreadyUnhiddenSelectors: Set<string>
      alreadyKnownFirstPartySubtrees: WeakSet
      _hasDelayOcurred: boolean
      _startCheckingId: number | undefined
    }
  }
}
