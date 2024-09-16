/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { LEO_DISTILLATION_LEVEL } from '../distillation'
import { getDistillationLevel } from './distiller'

export function isString(value: any) {
  return typeof value === 'string'
}

export function isSupportedPage(document: Document) {
  const supportedPaths = [
    '/',
    '/home',
    '/brave',
    '/bravesampson',
    '/brendaneich',
    '/notifications',
    /^\/.*?\/status\/\d+/ // Post detail pages
  ]

  for (const entry of supportedPaths) {
    if (entry instanceof RegExp && entry.test(document.location.pathname)) {
      return true
    } else if (isString(entry) && entry === document.location.pathname) {
      return true
    }
  }

  return false
}

/**
 * Decodes HTML entities in a given string using a
 * `textarea` element.
 */
export function decodeHTMLSpecialChars(text: string) {
  const textarea = document.createElement('textarea')
  /**
   * We have to use `.innerHTML` to decode entities, as
   * `.innerText` or `.textContent` will not achieve
   * the same results.
   */
  // eslint-disable-next-line no-unsanitized/property
  textarea.innerHTML = text
  return textarea.value
}

/**
 * Indents each line of the given text by a specified
 * number of spaces or a string prefix.
 */
export function indentLines(text: string, indent: number | string = 2) {
  const prefix = Number.isInteger(indent)
    ? ' '.repeat(indent as number)
    : indent
  return text
    .split('\n')
    .map((line) => `${prefix}${line}`)
    .join('\n')
}

/**
 * Wraps lines of text to a specified maximum length,
 * breaking at spaces to ensure readability.
 */
export function wrapLines(text: string, limit: number = 80) {
  return text
    .split('\n')
    .map((line) => {
      const words = line.split(' ')
      let replace = ''
      let length = 0
      for (const word of words) {
        if (length + word.length > limit) {
          replace += '\n'
          length = 0
        }
        replace += word + ' '
        length += word.length + 1
      }
      return replace.trim()
    })
    .join('\n')
}

export function msToMinutesAndSeconds(ms: number) {
  const minutes = Math.floor(ms / 60000)
  const seconds = Math.floor((ms % 60000) / 1000)
  return `${minutes}m ${seconds}s`
}

export function getDateString(date: any) {
  const dateObject = new Date(date)
  const distillLevel = getDistillationLevel()

  if (distillLevel < LEO_DISTILLATION_LEVEL.MEDIUM) {
    return dateObject.toISOString().substring(0, 10)
  }

  return dateObject.toLocaleString()
}

export function shortNumberString(value: number, precision: number = 1) {
  if (value < 1000) {
    return value.toString()
  }

  const suffixes = ['k', 'M', 'B', 'T']
  const suffixNum = Math.floor(('' + value).length / 3)
  let shortValue =
    suffixNum !== 0
      ? parseFloat((value / Math.pow(1000, suffixNum)).toPrecision(precision))
      : parseFloat(value.toPrecision(precision))

  if (shortValue % 1 !== 0) {
    shortValue = parseFloat(shortValue.toFixed(1))
  }

  return shortValue + suffixes[suffixNum - 1]
}
