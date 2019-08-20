/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Types
import { NoScriptInfo, NoScriptEntry } from '../types/other/noScriptInfo'

// Locale
import { getLocale } from '../background/api/localeAPI'

// Helpers
import { getOrigin, isHttpOrHttps } from './urlUtils'

/**
 * Filter resources by origin to be used for generating NoScriptInfo.
 * @param {NoScriptInfo} noScriptInfo - The NoScriptInfo state
 * @param {string} url - The URL to compare origins against each other
 * @returns {[string, NoScriptEntry][]} - The filtered list of scripts matching the same domain
 */
export const filterResourcesBySameOrigin = (noScriptInfo: NoScriptInfo, url: string): [string, NoScriptEntry][] => {
  return Object.entries(noScriptInfo).filter((script) => {
    return getOrigin(url) === getOrigin(script[0])
  })
}

/**
 * Generate data structure for the NoScript object.
 * This is useful to group scripts by origin and is used for presentational
 * purposes only, as data is stored same way as it comes from the back-end.
 * @param {NoScriptInfo} noScriptInfo - The NoScriptInfo state
 * @returns {Array<any>} - The new generated NoScriptInfo data
 */
export const generateNoScriptInfoDataStructure = (noScriptInfo: NoScriptInfo): Array<any> => {
  let newData = []
  for (const [url] of Object.entries(noScriptInfo)) {
    const entry = newData.some((item) => item[0] === getOrigin(url))
    if (!entry) {
      newData.push([ getOrigin(url), filterResourcesBySameOrigin(noScriptInfo, url) ])
    }
  }
  return newData
}

/**
 * Filter NoScriptInfo by `willBlock` state
 * @param {Array<any>} modifiedNoScriptInfo - The NoScriptInfo state
 * @param {boolean} maybeBlock - Whether or not the resource should be blocked
 * @returns {Array<any>} - The new generated NoScriptInfo data
 */
export const filterNoScriptInfoByWillBlockState = (
  modifiedNoScriptInfo: Array<any>,
  maybeBlock: boolean
): Array<any> => {
  return modifiedNoScriptInfo.filter(script => script[1].willBlock === maybeBlock)
}

/**
 * Check if all scripts in NoScriptInfo are either allowed or blocked by the user
 * @param {[string, NoScriptEntry][]} modifiedNoScriptInfo - The modifiedNoScriptInfo state
 * @param {boolean} isBlocked - Whether or not all scripts are blocked
 * @returns {boolean} - Whether or not the new generated NoScriptInfo data is either blocked or allowed
 */
export const checkEveryItemIsBlockedOrAllowedByUser = (
  modifiedNoScriptInfo: [string, NoScriptEntry][],
  isBlocked: boolean
): boolean => {
  return modifiedNoScriptInfo
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
  const allInteracted = checkEveryItemIsBlockedOrAllowedByUser(Object.entries(noScriptInfo), isBlocked)

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
  return getAllowedOrigins.map(url => isHttpOrHttps(url[0]) ? getOrigin(url[0]) + '/' : url[0])
}
