/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * Gets the localized string
 * @param {string} text - the locale string to translate
 * @returns {string} - the localized string
 */
export const getLocale = (text: string) => window.loadTimeData && window.loadTimeData.getString(text)
