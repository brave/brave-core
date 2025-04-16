/* Copyright (c) 2018 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { loadTimeData } from './loadTimeData'
import * as React from 'react'

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

interface SplitStringForTagResult {
  beforeTag: string
  duringTag: string | null
  afterTag: string | null
}

/**
 * Returns text for translations with a single HTML tag
 * (like a link or button)
 * @deprecated Use formatLocale instead.
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
 * @deprecated Use formatLocale instead.
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
 * @deprecated Use formatString instead.
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

type Replacement = string | React.ReactNode | ((contents: string) => React.ReactNode | string)
type ReturnType<T extends Replacement> = T extends (string | ((contents: string) => string)) ? string : React.ReactNode

/**
 * Formats a string with replacements. Replacements can be strings or React
 * Nodes. When you're interested in the content between tags you can pass a
 * function as a replacement - the content will be passed to the function as the
 * first argument and the return value will be used in the replacement.
 * @param text The text to format.
 * @param replacements The replacements to use. Can either be a string, a React Node or a function that returns a string or a React Node.
 * @returns The formatted string. If the replacements are all strings (or all return strings) the result will be a string. Otherwise the result will be a React Fragment.
 * @example
 * formatString('Hello $1', {
 *   $1: 'world'
 * }) // 'Hello world'
 *
 * formatString('Hello $1world$1', {
 *   $1: (content) => <a href="#">{content}</a>
 * }) // 'Hello <a href="#">world</a>'
 */
export function formatString<T extends Replacement>(text: string, replacements: Record<`$${number}`, T>): ReturnType<T> {
  const keysToReplace = Object.keys(replacements) as (keyof typeof replacements)[]
  const replacedRanges: ({
    start: number,
    end: number,
    value: string | React.ReactNode,
    key: keyof typeof replacements
  })[] = []

  let onlyText = true

  for (const key of keysToReplace) {
    const start = text.indexOf(key)

    // If we couldn't find the key the string or replacement is wrong.
    if (start === -1) throw new Error(`Replacement key ${key} not found in text`)

    // If we find an end tag, we need to replace the entire range.
    let end = text.indexOf(key, start + key.length)

    // If we don't find an end tag we just replace the key.
    if (end === -1) end = start

    const contents = text.substring(start + key.length, end)
      .replaceAll(key, '')

    // Make sure the range includes the entire key. We do this after getting the
    // contents so the contents doesn't include the key.
    end += key.length

    const value: React.ReactNode | string = typeof replacements[key] === 'function'
      ? replacements[key](contents)
      : replacements[key]

    // Keep track of whether we have any non-text replacements.
    onlyText = onlyText && typeof value === 'string'

    replacedRanges.push({
      start,
      end,
      value,
      key
    })
  }

  // Sort the ranges by start index so we can check for overlaps (which indicates a bug).
  replacedRanges.sort((a, b) => a.start - b.start)
  for (let i = 1; i < replacedRanges.length; i++) {
    if (replacedRanges[i].start < replacedRanges[i - 1].end) {
      throw new Error(`Replacement range ${replacedRanges[i].key}} overlaps with ${replacedRanges[i - 1].key}`)
    }
  }

  let lastIndex = 0
  let result: (string | React.ReactNode)[] = []
  for (const range of replacedRanges) {
    // Push the literal text before our replacement.
    if (lastIndex !== range.start) result.push(text.substring(lastIndex, range.start))

    // Push the replacement.
    result.push(range.value)

    // Update our last index.
    lastIndex = range.end
  }

  // Push the rest of the literal text.
  if (lastIndex !== text.length) result.push(text.substring(lastIndex))

  // If we only have text, return a string. Otherwise, wrap the result in a fragment.
  return (onlyText
    ? result.join('')
    : React.createElement(React.Fragment, null, ...result)) as any
}


/**
 * Formats a locale string with replacements. This is the same as formatString
 * but looks up the localized string from getLocale before formatting.
 * @param key The key for the localeString to format
 * @param replacements The replacements to use. Can either be a string, a React Node or a function that returns a string or a React Node.
 * @returns The formatted string. If the replacements are all strings (or all return strings) the result will be a string. Otherwise the result will be a React Fragment.
 * @example
 * formatLocale('shieldsStatus', {
 *   $1: status => <b>${status}</b>
 * }) // <>Shields are <b>UP</b></>
 */
export function formatLocale<T extends Replacement>(key: string, replacements: Record<`$${number}`, T>): ReturnType<T> {
  return formatString(getLocale(key), replacements)
}
