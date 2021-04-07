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
      returnVal = returnVal.replace(new RegExp('\\[\\[\\s*' + item + '\\s*\\]\\]'), replacements[item].toString())
    }
  }

  return returnVal
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
export const getLocaleWithTag = (key: string, replacements?: Record<string, string>) => {
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
export const getLocaleWithTags = (key: string, tags: number, replacements?: Record<string, string>) => {
  let text = getLocale(key, replacements)
  let result = []
  for (let i = 1; i <= tags; i++) {
    const split = splitStringForTag(text, (i * 2 - 1))
    text = split.afterTag || ''
    if (i !== tags) {
      split.afterTag = ''
    }
    result.push(split)
  }
  return result
}

/**
 * Allows an html or xml tag to be injected in to a string by extracting
 * the components of the string before, during and after the tag.
 * Usage:
 *    splitStringForTag('my string with some $1bold text$2')
 *
 * @export
 * @param {string} text
 * @param {number} tagStartNumber - starting number for the tag
 * @returns {SplitStringForTagResult}
 */
export function splitStringForTag (text: string, tagStartNumber: number = 1): SplitStringForTagResult {
  const tagOpenPlaceholder = `$${tagStartNumber}`
  const tagClosePlaceholder = `$${tagStartNumber + 1}`
  const tagStartIndex: number = text.indexOf(tagOpenPlaceholder)
  const tagEndIndex: number = text.lastIndexOf(tagClosePlaceholder)
  const isValid = (tagStartIndex !== -1 && tagEndIndex !== -1)
  const beforeTag = !isValid ? text : text.substring(0, tagStartIndex)
  const duringTag = isValid ? text.substring(tagStartIndex + tagOpenPlaceholder.length, tagEndIndex) : null
  const afterTag = isValid ? text.substring(tagEndIndex + tagClosePlaceholder.length) : null
  return {
    beforeTag,
    duringTag,
    afterTag
  }
}
