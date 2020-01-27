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
      returnVal = returnVal.replace(new RegExp('{{\\s*' + item + '\\s*}}'), replacements[item].toString())
    }
  }

  return returnVal
}

interface SplitStringForTagResult {
  beforeTag: string
  duringTag?: string
  afterTag?: string
}

/**
 * Allows an html or xml tag to be injected in to a string by extracting
 * the components of the string before, during and after the tag.
 * Usage:
 *    splitStringForTag('my string with some $1bold text$2', '$1', '$2')
 *
 * @export
 * @param {string} text
 * @param {string} tagOpenPlaceholder
 * @param {string} tagClosePlaceholder
 * @returns {SplitStringForTagResult}
 */
export function splitStringForTag (text: string, tagOpenPlaceholder: string, tagClosePlaceholder: string): SplitStringForTagResult {
  const tagStartIndex: number = text.indexOf(tagOpenPlaceholder)
  const tagEndIndex: number = text.lastIndexOf(tagClosePlaceholder)
  const isValid = (tagStartIndex !== -1 && tagEndIndex !== -1)
  const beforeTag = !isValid ? text : text.substring(0, tagStartIndex)
  const duringTag = isValid ? text.substring(tagStartIndex + tagOpenPlaceholder.length, tagEndIndex) : undefined
  const afterTag = isValid ? text.substring(tagEndIndex + tagClosePlaceholder.length) : undefined
  return {
    beforeTag,
    duringTag,
    afterTag
  }
}
