/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import rawMessages from '../../_locales/en_US/messages.json'

/**
 * Gets the locale message specified in messages.json
 * @param {string} message - The locale string
 */
export const getLocale = (messageStr: string): string => {
  if (chrome.i18n) {
    return chrome.i18n.getMessage(messageStr)
  }

  // If no string is available then get
  // the message from original (en-US) source.
  // Otherwise just output the string, which is a bug.
  return rawMessages[messageStr].message || messageStr
}
