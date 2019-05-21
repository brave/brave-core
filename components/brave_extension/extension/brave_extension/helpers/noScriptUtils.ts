/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Types
import { NoScriptInfo } from '../types/other/noScriptInfo'

// Locale
import { getLocale } from '../background/api/localeAPI'

/**
 * Check if all scripts in NoScriptInfo are either allowed or blocked by the user
 * @param {NoScriptInfo} noScriptInfo - The modifiedNoScriptInfo state
 * @param {boolean} isBlocked - Whether or not all scripts are blocked
 * @returns {boolean} - Whether or not the new generated NoScriptInfo data is either blocked or allowed
 */
export const checkEveryItemIsBlockedOrAllowedByUser = (
  modifiedNoScriptInfo: NoScriptInfo,
  isBlocked: boolean
): boolean => {
  return Object.entries(modifiedNoScriptInfo)
    .filter(script => script[1].willBlock === isBlocked)
    .every(script => script[1].userInteracted)
}

/**
 * Get script "block all"/"allow all" text
 * Scripts are divided between blocked/allowed and we have an option to block/allow all.
 * If all scripts in a list are set to blocked/allowed, state should change
 * to "allowed once" or "blocked once"
 * @param {NoScriptInfo} noScriptInfo - The NoScriptInfo state
 * @param {boolean} isBlocked - Whether or not all scripts are blocked
 * @returns {string} - The string to be used by the string
 */
export const getBlockAllText = (
  noScriptInfo: NoScriptInfo,
  isBlocked: boolean
): string => {
  const allInteracted = checkEveryItemIsBlockedOrAllowedByUser(noScriptInfo, isBlocked)

  if (isBlocked) {
    if (allInteracted) {
      return getLocale('allowedOnce')
    }
    return getLocale('allowAll')
  } else {
    if (allInteracted) {
      return getLocale('blockedOnce')
    }
    return getLocale('blockAll')
  }
}

/**
 * Get script "block" text
 * Scripts can be set as allow/block when there is no interaction
 * and allowed once/blocked once when interaction have happened
 * @param {boolean} haveUserInteracted - Whether or not user have interacted with the script
 * @param {boolean} isBlocked - Whether or not the current script is blocked
 * @returns {string} - The string to be used by the allowed/blocked group
 */
export const getBlockScriptText = (haveUserInteracted: boolean, isBlocked: boolean): string => {
  if (!haveUserInteracted) {
    return isBlocked ? getLocale('allow') : getLocale('block')
  }
  return isBlocked ? getLocale('allowedOnce') : getLocale('blockedOnce')
}

/**
 * Get script origins allowed by the user
 * @param {NoScriptInfo} noScriptInfo - The modifiedNoScriptInfo state
 * @param {boolean} isBlocked - Whether or not all scripts are blocked
 * @returns {Array<string>} - An array with all origins that user decided to allow
 */
export const getAllowedScriptsOrigins = (modifiedNoScriptInfo: NoScriptInfo): Array<string> => {
  const getAllowedOrigins = Object.entries(modifiedNoScriptInfo)
    .filter(url => url[1].actuallyBlocked === false)
  return getAllowedOrigins.map(url => url[0])
}
