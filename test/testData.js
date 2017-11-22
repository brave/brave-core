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

export const getMockChrome = () => {
  const onBlockedListeners = []
  const onConnectListeners = []
  const onMessageListeners = []
  return {
    runtime: {
      onMessage: {
        addListener: function (cb) {
          onMessageListeners.push(cb)
        }
      },
      onConnect: {
        addListener: function (cb) {
          onConnectListeners.push(cb)
        }
      }
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
      }
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
    }
  }
}

export const initialState = {
  shieldsPanel: {
    tabs: {}
  },
  newTabPage: {}
}
