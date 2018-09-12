/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

interface ResolveInfo {
  resultCode: number
  address?: string
}

/*
 * Shimming node's dns.lookup API using chrome.dns.resolve.
 * @param {string} hostname: the hostname to be resolved
 * @param {function} cb: a callback function for dns.lookup with error and ip arguments
 */
export const lookup = (hostname: string, cb: any) => {
  chrome.dns.resolve(hostname, (resolveInfo: ResolveInfo) => {
    if (resolveInfo.resultCode !== 0) {
      cb(new Error('chrome.dns.resolve error'), undefined)
    } else {
      cb(undefined, resolveInfo.address)
    }
  })
}
