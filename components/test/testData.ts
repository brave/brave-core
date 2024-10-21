/* Copyright (c) 2018 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Initial state
import { defaultState as rewardsData } from '../../components/brave_rewards/resources/page/reducers/default_state'
import { defaultState as adblockData } from '../../components/brave_adblock_ui/storage'

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

export const rewardsInitialState: Rewards.ApplicationState = { rewardsData }

export const adblockInitialState: AdBlock.ApplicationState = { adblockData }

export const newTabInitialState: NewTab.ApplicationState = {
  newTabData: {
    showBackgroundImage: false,
    showSettingsMenu: false,
    topSites: [],
    excludedSites: [],
    gridSites: [],
    showEmptyPage: false,
    isIncognito: new ChromeEvent(),
    useAlternativePrivateSearchEngine: false,
    showAlternativePrivateSearchEngineToggle: false,
    torCircuitEstablished: false,
    torInitProgress: '',
    isTor: false,
    stats: {
      adsBlockedStat: 0,
      javascriptBlockedStat: 0,
      bandwidthSavedStat: 0,
      fingerprintingBlockedStat: 0
    }
  }
}

// see: https://developer.chrome.com/extensions/events
interface OnMessageEvent extends chrome.events.Event<(message: object, options: any, responseCallback: any) => void> {
  emit: (message: object) => void
}

export const getMockChrome = () => {
  let mock = {
    send: (methodName: string, ...args: any[]) => undefined,
    getVariableValue: () => undefined,
    runtime: {
      onMessage: new ChromeEvent(),
      onConnect: new ChromeEvent(),
      onStartup: new ChromeEvent(),
      onMessageExternal: new ChromeEvent(),
      onConnectExternal: new ChromeEvent(),
      // see: https://developer.chrome.com/apps/runtime#method-sendMessage
      sendMessage: function (message: object, responseCallback: () => void) {
        const onMessage = chrome.runtime.onMessage as OnMessageEvent
        onMessage.emit(message)
        responseCallback()
      }
    },
    browserAction: {
      setBadgeBackgroundColor: function (properties: object) {

      },
      setBadgeText: function (textProperties: object) {

      },
      setIcon: function (iconProperties: object) {

      },
      enable: function (tabId?: number) {

      },
      disable: function (tabId?: number) {

      }
    },
    tabs: {
      create: function (createProperties: object, cb: () => void) {
        setImmediate(cb)
      },
      reload: function (tabId: number, reloadProperties: object, cb: () => void) {
        setImmediate(cb)
      },
      insertCSS: function (details: jest.SpyInstance) {

      },
      query: function (queryInfo: chrome.tabs.QueryInfo, callback: (result: chrome.tabs.Tab[]) => void) {
        return callback
      },
      sendMessage: function (tabID: Number, message: any, options: object, responseCallback: any) {
        return responseCallback
      },
      onActivated: new ChromeEvent(),
      onCreated: new ChromeEvent(),
      onUpdated: new ChromeEvent(),
      onRemoved: new ChromeEvent()
    },
    windows: {
      onFocusChanged: new ChromeEvent(),
      onCreated: new ChromeEvent(),
      onRemoved: new ChromeEvent(),
      getAllAsync: function () {
        return new Promise(() => [])
      }
    },
    i18n: {
      getMessage: function (message: string) {

      }
    },
    storage: {
      local: {
        get: function (callback: (items: { [key: string]: any }) => void): void {

        },
        set: function (items: Object, callback?: () => void): void {

        }
      }
    },
    extension: {
      inIncognitoContext: new ChromeEvent()
    },
    topSites: {
      get: function (cb) {
        cb([])
      }
    },
    bookmarks: {
      create: function (bookmark: chrome.bookmarks.BookmarkCreateArg, callback?: (result: chrome.bookmarks.BookmarkTreeNode[]) => void) {

      },
      remove: function (id: string, callback?: Function) {

      },
      search: function (query: string, callback: (results: chrome.bookmarks.BookmarkTreeNode[]) => void) {

      }
    },
    contextMenus: {
      create: function (data: any) {
        return Promise.resolve()
      },
      allowScriptsOnce: function (origins: string[], tabId: number, cb: () => void) {
        setImmediate(cb)
      },
      onClicked: new ChromeEvent()
    }
  }
  return mock
}

export const getMockLoadTimeData = () => {
  return {
    getString(key) {
      if (key === 'braveWalletLedgerBridgeUrl') {
        return 'chrome-untrusted://ledger-bridge'
      }
      return key
    },
    getBoolean() {
      return true
    },
    getInteger() {
      return 2
    },
    getStringF(key) {
      return key
    }
  }
}

export const window = () => {
  let mock = {
    prompt: function (text: String) {
      return text
    }
  }
  return mock
}

export const mockSearchProviders = [
  {
    canBeDefault: true,
    canBeEdited: true,
    canBeRemoved: true,
    default: true,
    displayName: 'DuckDuckGo',
    iconURL: 'https://duckduckgo.com/favicon.ico',
    id: 3,
    isOmniboxExtension: false,
    keyword: ':d',
    modelIndex: 1,
    name: 'DuckDuckGo',
    url: 'https://duckduckgo.com/?q=%s&t=brave',
    urlLocked: true
  }
]

export const mockImportSources = [
  {
    autofillFormData: false,
    cookies: true,
    favorites: true,
    history: true,
    index: 1,
    ledger: false,
    name: 'Chrome Person 1',
    passwords: true,
    search: false,
    stats: false,
    windows: false
  },
  {
    autofillFormData: false,
    cookies: true,
    favorites: true,
    history: true,
    index: 0,
    ledger: false,
    name: 'Safari',
    passwords: true,
    search: false,
    stats: false,
    windows: false
  },
  {
    autofillFormData: false,
    cookies: true,
    favorites: true,
    history: true,
    index: 2,
    ledger: false,
    name: 'Bookmarks HTML File',
    passwords: true,
    search: false,
    stats: false,
    windows: false
  }
]
