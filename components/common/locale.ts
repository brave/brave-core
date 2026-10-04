/* Copyright (c) 2018 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { loadTimeData } from './loadTimeData'
import { formatString, Replacement } from './formatString'

/**
 * Gets the localized string
 * @param {string} key - translation identifier
 * @returns {string} - the localized string
 */
export const getLocale = (key: string) => {
  if (!key) {
    console.error('locale string requires a key!')
    return key
  }

  const returnVal = loadTimeData.getString(key)
  if (!returnVal) {
    console.error(`locale string not found for key: ${key}`)
    return key
  }

  return returnVal
}

/**
 * Formats a locale string with replacements. This is the same as formatString
 * but looks up the localized string from getLocale before formatting.
 * @param key The key for the localeString to format
 * @param replacements The replacements to use. Can either be a string, a React Node or a function that returns a string or a React Node.
 * @returns The formatted string. If the replacements are all strings (or all return strings) the result will be a string. Otherwise the result will be a React Fragment.
 * @example
 * formatLocale('shieldsStatus', {
 *   $1: status => <b>${status}</b>
 * }) // <>Shields are <b>UP</b></>
 */
export function formatLocale<T extends Replacement>(
  key: string,
  replacements: Parameters<typeof formatString<T>>[1],
  options?: Parameters<typeof formatString<T>>[2]
) {
  return formatString<T>(getLocale(key), replacements, options)
}
