/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export function isEmpty (obj: object) {
  return Object.entries(obj).length === 0 && obj.constructor === Object
}

export const statusCode: Array<any> = [
  404, 408, 410, 451, 500,
  502, 503, 504, 509, 520,
  521, 523, 524, 525, 526
]
