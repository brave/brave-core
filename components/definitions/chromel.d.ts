/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

declare namespace chrome {
  function getVariableValue (variable: string): string
  function setVariableValue (variable: string, value: any): void
  function send (stat: string, args?: any[]): void
}

declare namespace chrome.dns {
  function resolve (hostname: string, callback: any): void
}

declare namespace chrome.braveRewards {
  const createWallet: () => {}
  const donateToSite: (tabId: number, publisherKey: string) => {}
  const getPublisherData: (windowId: number, url: string) => {}
  const getWalletProperties: () => {}
  const getCurrentReport: () => {}
  const onWalletCreated: {
    addListener: (callback: () => void) => void
  }
  const onPublisherData: {
    addListener: (callback: (windowId: number, publisher: RewardsExtension.Publisher) => void) => void
  }
  const onWalletProperties: {
    addListener: (callback: (properties: RewardsExtension.WalletProperties) => void) => void
  }
  const onCurrentReport: {
    addListener: (callback: (properties: RewardsExtension.Report) => void) => void
  }
}

declare namespace chrome.rewardsNotifications {
  const addNotification: (type: number, args: string[]) => {}
  const deleteNotification: (id: number) => {}
  const deleteAllNotifications: () => {}
  const getNotification: (id: number) => {}

  const onNotificationAdded: {
    addListener: (callback: (id: number, type: number, timestamp: number, args: string[]) => void) => void
  }
  const onNotificationDeleted: {
    addListener: (callback: (id: number, type: number, timestamp: number) => void) => void
  }
  const onAllNotificationsDeleted: {
    addListener: (callback: () => void) => void
  }
  const onGetNotification: {
    addListener: (callback: (id: number, type: number, timestamp: number) => void) => void
  }
}
