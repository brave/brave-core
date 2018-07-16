/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

'use strict'

export const debounce = function<T>(fn: (data: T) => void, bufferInterval: number, ...args: Array<any>) {
  let timeout: any
  return (...args2: any[]) => {
    clearTimeout(timeout)
    let a: Array<string> = args || []
    if (args2 && args2.constructor === Array) {
      a = a.concat(args2)
    }
    timeout = setTimeout(fn.apply.bind(fn, this, a), bufferInterval)
  }
}
