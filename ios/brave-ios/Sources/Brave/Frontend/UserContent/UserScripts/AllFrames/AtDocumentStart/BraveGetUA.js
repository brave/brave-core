/* vim: set ts=2 sts=2 sw=2 et tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

'use strict'

Object.defineProperty(navigator, 'brave', {
  enumerable: false,
  configurable: true,
  writable: false,
  value: Object.freeze({
    isBrave: () => new Promise(res => res(true))
  })
})
