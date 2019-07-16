/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { NoScriptInfoInterface } from './types'
import { getLocale } from './fakeLocale'

export const getTabIndexValueBasedOnProps = (
  isBlockedListOpen: boolean,
  numberOfBlockedItems: number
): number => {
  return (isBlockedListOpen || numberOfBlockedItems === 0) ? -1 : 0
}

/**
 * Get the URL origin via Web API
 * @param {string} url - The URL to get the origin from
 */
export const getOrigin = (url: string) => new window.URL(url).origin

/**
 * Get the URL hostname via Web API
 * @param {string} url - The URL to get the origin from
 */
export const getHostname = (url: string) => new window.URL(url).hostname

/**
 * Strip http/https protocol
 * @param {string} url - The URL to strip the protocol from
 */
export const stripProtocolFromUrl = (url: string) => url.replace(/(^\w+:|^)\/\//, '')

/**
 * Filter resources by origin to be used for generating NoScriptInfo.
 * @param {[key: string]: NoScriptInfoInterface} noScriptInfo - The NoScriptInfo state
 * @param {string} url - The URL to compare origins against each other
 */
export const filterResourceByOrigin = (noScriptInfo: { [key: string]: NoScriptInfoInterface }, url: string) => {
  return Object.entries(noScriptInfo).filter((script) => {
    return getOrigin(url) === getOrigin(script[0])
  })
}

/**
 * Generate data structure for the NoScript object.
 * This is useful to group scripts by origin and is used for presentational
 * purposes only, as data is stored same way as it comes from the back-end.
 * @param {[key: string]: NoScriptInfoInterface} noScriptInfo - The NoScriptInfo state
 */
export const generateNoScriptInfoDataStructure = (noScriptInfo: { [key: string]: NoScriptInfoInterface }) => {
  let newData = []
  for (const [url] of Object.entries(noScriptInfo)) {
    const entry = newData.some((item) => item[0] === getOrigin(url))
    if (!entry) {
      newData.push([ getOrigin(url), filterResourceByOrigin(noScriptInfo, url) ])
    }
  }
  return newData
}

/**
 * Filter NoScriptInfo by `willBlock` state
 * @param {[key: string]: NoScriptInfoInterface} noScriptInfo - The NoScriptInfo state
 * @param {boolean} maybeBlock - Whether or not the resource should be blocked
 * @param {boolean} filterByDifference - Whether or not `willBlock` should be filtered by difference
 */
export const filterNoScriptInfoByBlockedState = (
  noScriptInfo: Array<any>,
  maybeBlock: boolean,
  filterByDifference?: boolean
) => {
  if (filterByDifference) {
    return noScriptInfo.filter(script => script[1].willBlock !== maybeBlock)
  }
  return noScriptInfo.filter(script => script[1].willBlock === maybeBlock)
}

/**
 * Check if all scripts in NoScriptInfo are either allowed or blocked by the user
 * @param {[key: string]: NoScriptInfoInterface} noScriptInfo - The NoScriptInfo state
 * @param {boolean} isBlocked - Whether or not all scripts are blocked
 */
export const checkEveryItemIsBlockedOrAllowed = (
  noScriptInfo: Array<any>,
  isBlocked: boolean,
  shouldParseData?: boolean
) => {
  if (shouldParseData) {
    return Object.entries(noScriptInfo)
      .filter(script => script[1].willBlock === isBlocked)
      .every(script => script[1].userInteracted)
  }
  return noScriptInfo
    .filter(script => script[1].willBlock === isBlocked)
    .every(script => script[1].userInteracted)
}

/**
 * Get script "block all"/"allow all" text
 * Scripts are divided between blocked/allowed and we have an option to block/allow all.
 * If all scripts in a list are set to blocked/allowed, state should change
 * to "allowed once" or "blocked once"
 * @param {[key: string]: NoScriptInfoInterface} noScriptInfo - The NoScriptInfo state
 * @param {boolean} isBlocked - Whether or not all scripts are blocked
 */
export const getBlockAllText = (
  noScriptInfo: { [key: string]: NoScriptInfoInterface },
  isBlocked: boolean
) => {
  const everyItemIsWasInteracted = Object.entries(noScriptInfo)
    .every(data => data[1].willBlock === isBlocked && data[1].userInteracted)

  if (isBlocked) {
    if (everyItemIsWasInteracted) {
      return getLocale('allowedOnce')
    }
    return getLocale('allowAll')
  } else {
    if (everyItemIsWasInteracted) {
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
 */
export const getBlockScriptText = (haveUserInteracted: boolean, isBlocked: boolean) => {
  if (!haveUserInteracted) {
    return isBlocked ? getLocale('allow') : getLocale('block')
  }
  return isBlocked ? getLocale('allowedOnce') : getLocale('blockedOnce')
}
