/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export const activeTabData = {
  id: 2,
  url: 'https://www.brave.com'
}

export const activeTabShieldsData = {
  origin: 'https://www.brave.com',
  hostname: 'www.brave.com',
  adBlock: 'block',
  trackingProtection: 'block',
  tabId: 2
}

export const blockedResource = {
  blockType: 'adBlock',
  tabId: 2
}

class ChromeEvent {
  constructor () {
    this.listeners = []
  }
  emit () {
    this.listeners.forEach((cb) => cb())
  }
  addListener (cb) {
    this.listeners.push(cb)
  }
}

export const getMockChrome = () => {
  const onBlockedListeners = []
  return {
    runtime: {
      onMessage: new ChromeEvent(),
      onConnect: new ChromeEvent()
    },
    browserAction: {
      setBadgeText: function (text) {
      }
    },
    tabs: {
      queryAsync: function () {
        return Promise.resolve([activeTabData])
      },
      create: function (createProperties, cb) {
        setImmediate(cb)
      },
      onActivated: new ChromeEvent(),
      onUpdated: new ChromeEvent()
    },
    braveShields: {
      onBlocked: {
        addListener: function (cb) {
          onBlockedListeners.push(cb)
        },
        emit: function () {
          onBlockedListeners.forEach((cb) => cb(blockedResource))
        }
      }
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
      }
    },
    i18n: {
      getMessage: function (message) {
      }
    }
  }
}

export const initialState = {
  shieldsPanel: {
    tabs: {}
  },
  newTabPage: {}
}
