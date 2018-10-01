/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
export {}

type loadTimeData = {
  getString: (key: string) => string
  data_: Record<string, string>
}

declare global {
  interface Window {
    loadTimeData: loadTimeData
    cr: {
      define: (name: string, init: () => void) => void
    }
    i18nTemplate: {
      process: (document: Document, translations: loadTimeData) => void
    }
    brave_adblock: {
      initialize: () => void
    }
    brave_new_tab: {
      initialize: () => void
      statsUpdated: () => void
    }
    brave_rewards: {
      initialize: () => void
      walletCreated: chrome.events.Event<() => void>
      walletCreateFailed: chrome.events.Event<() => void>
      walletProperties: chrome.events.Event<(properties: {status: number, wallet: Rewards.WalletProperties}) => void>
      grant: chrome.events.Event<(properties: Rewards.Grant) => void>
      grantCaptcha: chrome.events.Event<(image: string) => void>
      walletPassphrase: chrome.events.Event<(pass: string) => void>
      recoverWalletData: chrome.events.Event<(properties: Rewards.RecoverWallet) => void>
      grantFinish: chrome.events.Event<(properties: Rewards.GrantFinish) => void>
      reconcileStamp: chrome.events.Event<(stamp: number) => void>
      addresses: chrome.events.Event<(addresses: Record<string, string>) => void>
      contributeList: chrome.events.Event<(list: Rewards.Publisher[]) => void>
      balanceReports: chrome.events.Event<(reports: Record<string, Rewards.Report>) => void>
    }
    brave_welcome: {
      initialize: () => void
    }
  }
}
