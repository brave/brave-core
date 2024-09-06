/* Copyright (c) 2018 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { loadTimeData } from './loadTimeData'

/**
 * Gets the localized string
 * @param {string} key - translation identifier
 * @param {object} replacements - replacements for specific translation, replacement should be defined as {{key}}
 * @returns {string} - the localized string
 */
export const getLocale = (
  key: string,
  replacements?: Record<string, string>
) => {
  if (!key) {
    console.error('locale string requires a key!')
    return key
  }

  let returnVal = loadTimeData.getString(key)
  if (!returnVal) {
    console.error(`locale string not found for key: ${key}`)
    return key
  }

  if (replacements) {
    for (let item in replacements) {
      returnVal = returnVal.replace(
        new RegExp('\\[\\[\\s*' + item + '\\s*\\]\\]'),
        replacements[item].toString()
      )
    }
  }

  return returnVal
}

/**
 * Gets the localized string
 * @param {string} key - translation identifier
 * @param {object} replacements - replacements for specific key
 * @returns {string} - the localized string
 * Usage:
 *    const replacement = {
 *      $1: 10,
 *      $2: 20
 *    }
 *    getLocaleWithReplacements('$1 cities, $2 servers', replacement)
 *    returns '10 cities, 20 servers`.
 */
export function getLocaleWithReplacements(
  key: string,
  replacements: { [key: `${number}`]: string }
) {
  return getLocale(key).replace(/\$\d+/g, (m) => replacements[m])
}

interface SplitStringForTagResult {
  beforeTag: string
  duringTag: string | null
  afterTag: string | null
}

/**
 * Returns text for translations with a single HTML tag
 * (like a link or button)
 * @param {string} key - translation identifier
 * @param {object} replacements - replacements for specific translation, replacement should be defined as {{key}}
 * @returns {SplitStringForTagResult}
 */
export const getLocaleWithTag = (
  key: string,
  replacements?: Record<string, string>
) => {
  const text = getLocale(key, replacements)
  return splitStringForTag(text)
}

/**
 * Returns text for translations with a multiple HTML tags
 * (like a link or button)
 * @param {string} key - translation identifier
 * @param {number} tags - how many tags is in translation
 * @param {object} replacements - replacements for specific translation, replacement should be defined as {{key}}
 * @returns {SplitStringForTagResult}
 */
export const getLocaleWithTags = (
  key: string,
  tags: number,
  replacements?: Record<string, string>
) => {
  let text = getLocale(key, replacements)
  let result = []
  for (let i = 1; i <= tags; i++) {
    const split = splitStringForTag(text, i * 2 - 1)
    text = split.afterTag || ''
    if (i !== tags) {
      split.afterTag = ''
    }
    result.push(split)
  }
  return result
}

/**
 * Allows an html or xml tag, or React component etc., to be injected in to a string by extracting
 * the components of the string before, during and after the tag.
 * Usage:
 *    splitStringForTag('my string with some $1bold text$2')
 *    splitStringForTag('Get more info at $1')
 *
 * @export
 * @param {string} text
 * @param {number} tagStartNumber - starting number for the tag
 * @returns {SplitStringForTagResult}
 */
export function splitStringForTag(
  text: string,
  tagStartNumber: number = 1
): SplitStringForTagResult {
  const tagOpenPlaceholder = `$${tagStartNumber}`
  const tagClosePlaceholder = `$${tagStartNumber + 1}`
  const tagStartIndex: number = text.indexOf(tagOpenPlaceholder)
  const tagEndIndex: number = text.lastIndexOf(tagClosePlaceholder)
  const isValid = tagStartIndex !== -1
  let beforeTag = text
  let duringTag: string | null = null
  let afterTag: string | null = null
  if (isValid) {
    beforeTag = text.substring(0, tagStartIndex)
    if (tagEndIndex !== -1) {
      // Handle we have 'open' and 'close' tags
      duringTag = text.substring(
        tagStartIndex + tagOpenPlaceholder.length,
        tagEndIndex
      )
      afterTag = text.substring(tagEndIndex + tagClosePlaceholder.length)
    } else {
      // Handle we have a single replacement block
      afterTag = text.substring(tagStartIndex + tagOpenPlaceholder.length)
    }
  }

  return {
    beforeTag,
    duringTag,
    afterTag
  }
}
