/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as bluebird from 'bluebird'

import './background/greaselion'
import './background/webDiscoveryProject'

// workaround https://github.com/DefinitelyTyped/DefinitelyTyped/issues/42084
global.Promise = bluebird as any

function promisifier (method: any) {
  // return a function
  return function promisified (...args: any[]) {
    // which returns a promise
    return new Promise((resolve: any) => {
      args.push(resolve)
      method.apply(this, args)
    })
  }
}

function promisifyAll (obj: object, list: string[]) {
  list.forEach(api => bluebird.promisifyAll(obj[api], { promisifier }))
}

// let chrome extension api support Promise
promisifyAll(chrome, [
  'browserAction',
  'tabs',
  'windows'
])

promisifyAll(chrome.storage, [
  'local'
])

bluebird.promisifyAll(chrome.braveShields, { promisifier })

require('./background/api')
require('./background/events')
require('./background/store')
if (chrome.test) {
  chrome.test.sendMessage('brave-extension-enabled')
}

function cleanUp () {
  // 2021-10 Removes Brave News local storage now that backend has migrated
  // to browser.
  chrome.storage.local.remove(['today', 'todayEtag', 'todayLastRemoteUpdateCheck', 'todayPublishers'])
}

chrome.runtime.onStartup.addListener(function () {
  // No need to do storage cleanup immediately on startup
  setTimeout(cleanUp, 60000)
})
