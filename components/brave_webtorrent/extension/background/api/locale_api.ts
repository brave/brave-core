/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * Gets the locale message specified in messages.json
 * @param {string} message - The locale string
 */
 export const getMessage = (message: string, substitutions?: string[]): string => {
    if (chrome.i18n) {
      let translated = chrome.i18n.getMessage(message, substitutions)

      if (translated) {
        return translated
      } else {
        return `i18n: ${message}`
      }
    }

    return `i18n missing: ${message}`
  }
