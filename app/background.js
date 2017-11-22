/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const bluebird = require('bluebird')
global.Promise = bluebird

function promisifier (method) {
  // return a function
  return function promisified (...args) {
    // which returns a promise
    return new Promise((resolve) => {
      args.push(resolve)
      method.apply(this, args)
    })
  }
}

function promisifyAll (obj, list) {
  list.forEach(api => bluebird.promisifyAll(obj[api], { promisifier }))
}

// let chrome extension api support Promise
promisifyAll(chrome, [
  'tabs',
  'windows',
  'browserAction'
])
promisifyAll(chrome.storage, [
  'local'
])

promisifyAll(chrome.contentSettings, [
  'braveAdBlock',
  'braveTrackingProtection'
])

require('./background/events/tabsEvents')
require('./background/events/shieldsEvents')
require('./background/store')
