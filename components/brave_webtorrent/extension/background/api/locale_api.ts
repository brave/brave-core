/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
import type Messages from '../../_locales/en_GB/messages.json'


type MessageKeys = (keyof typeof Messages) & string
/**
 * Gets the locale message specified in messages.json
 * @param {string} message - The locale string
 */
 export const getMessage = (message: MessageKeys, substitutions?: string[]): string => {
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
