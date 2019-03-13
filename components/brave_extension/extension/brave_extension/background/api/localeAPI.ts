/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * Gets the locale message specified in messages.json
 * @param {string} message - The locale string
 */
export const getLocale = (message: string): string => {
  if (chrome.i18n) {
    return chrome.i18n.getMessage(message)
  }

  return message
}
