/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as deepFreeze from 'deep-freeze-node'
import { Tab } from '../app/types/state/shieldsPannelState'
import { BlockDetails } from '../app/types/actions/shieldsPanelActions'

interface CustomTab extends Tab {
  url: string
}

interface Tabs {
  [key: number]: CustomTab
}

export const tabs: Tabs = {
  2: {
    id: 2,
    url: 'https://www.brave.com/test',
    origin: 'https://www.brave.com',
    hostname: 'www.brave.com',
    ads: 'block',
    adsBlocked: 0,
    trackers: 'block',
    httpUpgradableResources: 'block',
    javascript: 'block',
    fingerprinting: 'block',
    cookies: 'block',
    controlsOpen: false,
    httpsRedirected: 0,
    javascriptBlocked: 0,
    fingerprintingBlocked: 0,
    braveShields: 'block',
    trackersBlocked: 0,
    noScriptInfo: {},
    adsBlockedResources: [],
    trackersBlockedResources: [],
    httpsRedirectedResources: [],
    javascriptBlockedResources: [],
    fingerprintingBlockedResources: []
  }
}

export const activeTabData = tabs[2]

export const blockedResource: BlockDetails = {
  blockType: 'ads',
  tabId: 2,
  subresource: 'https://www.brave.com/test'
}

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
    runtime: {
      onMessage: new ChromeEvent(),
      onConnect: new ChromeEvent(),
      onStartup: new ChromeEvent()
    },
    browserAction: {
      setBadgeBackgroundColor: function (properties: object) {},
      setBadgeText: function (textProperties: object) {},
      setIcon: function (iconProperties: object) {},
      enable: function (tabId?: number) {},
      disable: function (tabId?: number) {}
    },
    tabs: {
      queryAsync: function () {
        return Promise.resolve([activeTabData])
      },
      getAsync: function (tabId: number) {
        return Promise.resolve(tabs[tabId])
      },
      create: function (createProperties: object, cb: () => void) {
        setImmediate(cb)
      },
      reload: function (tabId: number, reloadProperties: object, cb: () => void) {
        setImmediate(cb)
      },
      insertCSS: function (details: any) {},
      onActivated: new ChromeEvent(),
      onCreated: new ChromeEvent(),
      onUpdated: new ChromeEvent()
    },
    windows: {
      onFocusChanged: new ChromeEvent(),
      onCreated: new ChromeEvent(),
      onRemoved: new ChromeEvent(),
      getAllAsync: function () {
        return new Promise(() => [])
      }
    },
    braveShields: {
      onBlocked: new ChromeEvent(),
      allowScriptsOnce: function (origins: Array<string>, tabId: number, cb: () => void) {
        setImmediate(cb)
      }
    },
    contentSettings: {
      plugins: {
        setAsync: function () {
          return Promise.resolve()
        },
        getAsync: function () {
          return Promise.resolve({
            setting: 'block'
          })
        }
      },
      javascript: {
        setAsync: function () {
          return Promise.resolve()
        },
        getAsync: function () {
          return Promise.resolve({
            setting: 'block'
          })
        }
      }
    },
    i18n: {
      getMessage: function (message: string) {}
    },
    storage: {
      local: {
        get: function (url: string) {},
        set: function (url: string, cssfilter: string) {}
      }
    }
  }
}

export const initialState = deepFreeze({
  cosmeticFilter: {
    currentWindowId: -1,
    tabs: {},
    windows: {}
  },
  runtime: {},
  shieldsPanel: {
    currentWindowId: -1,
    tabs: {},
    windows: {}
  }
})
