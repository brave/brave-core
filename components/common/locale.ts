/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * Gets the localized string
 * @param {string} key - translation identifier
 * @param {object} replacements - replacements for specific translation, replacement should be defined as {{key}}
 * @returns {string} - the localized string
 */
export const getLocale = (key: string, replacements?: Record<string, string>) => {
  if (!key || !window.loadTimeData) {
    return key
  }

  let returnVal = window.loadTimeData.getString(key)
  if (!returnVal) {
    return key
  }

  if (replacements) {
    for (let item in replacements) {
      returnVal = returnVal.replace(new RegExp('{{\\s*' + item + '\\s*}}'), replacements[item].toString())
    }
  }

  return returnVal
}
