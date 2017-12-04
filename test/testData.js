/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import deepFreeze from 'deep-freeze-node'

export const tabs = {
  2: {
    id: 2,
    url: 'https://www.brave.com/test',
    origin: 'https://www.brave.com',
    hostname: 'www.brave.com',
    adBlock: 'block',
    trackingProtection: 'block',
    httpsEverywhere: 'block',
    javascript: 'block'
  }
}

export const activeTabData = tabs[2]

export const blockedResource = {
  blockType: 'adBlock',
  tabId: 2
}

class ChromeEvent {
  constructor () {
    this.listeners = []
  }
  emit (...args) {
    this.listeners.forEach((cb) => cb.apply(null, args))
  }
  addListener (cb) {
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
      setBadgeText: function (text) {
      }
    },
    tabs: {
      queryAsync: function () {
        return Promise.resolve([activeTabData])
      },
      getAsync: function (tabId) {
        return Promise.resolve(tabs[tabId])
      },
      create: function (createProperties, cb) {
        setImmediate(cb)
      },
      reload: function (tabId, bypassCache, cb) {
        setImmediate(cb)
      },
      onActivated: new ChromeEvent(),
      onCreated: new ChromeEvent(),
      onUpdated: new ChromeEvent()
    },
    windows: {
      onFocusChanged: new ChromeEvent(),
      onCreated: new ChromeEvent(),
      onRemoved: new ChromeEvent(),
      getAllAsync: function () {
        return new Promise([])
      }
    },
    braveShields: {
      onBlocked: new ChromeEvent()
    },
    contentSettings: {
      braveAdBlock: {
        setAsync: function () {
          return Promise.resolve()
        },
        getAsync: function () {
          return Promise.resolve({
            setting: 'block'
          })
        }
      },
      braveTrackingProtection: {
        setAsync: function () {
          return Promise.resolve()
        },
        getAsync: function () {
          return Promise.resolve({
            setting: 'block'
          })
        }
      },
      braveHTTPSEverywhere: {
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
      getMessage: function (message) {
      }
    }
  }
}

export const initialState = deepFreeze({
  shieldsPanel: {
    tabs: {},
    windows: {}
  },
  newTabPage: {}
})
