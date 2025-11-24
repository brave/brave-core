// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

type Content = string | React.ReactNode
export type Replacement = Content | ((contents: string) => Content)
type ReturnType<T extends Replacement> = T extends (string | ((contents: string) => string)) ? string : React.ReactNode
type Options = {
  noErrorOnMissingReplacement?: boolean
}

interface ReplacedRange {
  start: number
  end: number
  key: `$${number}`
  children: ReplacedRange[]
  hasClosingTag: boolean
}

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
 * formatString('Hello $1world/$1', {
 *   $1: (content) => <a href="#">{content}</a>
 * }) // 'Hello <a href="#">world</a>'
 */
export function formatString<T extends Replacement>(
  text: string,
  replacementsRaw: Record<`$${string}`, T> | T[],
  options?: Options
): ReturnType<T> {
  // If the replacements are an array, convert them to an object with the
  // correct keys (0 ==> $1, 1 ==> $2, etc).
  const replacements = Array.isArray(replacementsRaw)
    ? replacementsRaw.reduce<Record<`$${string}`, T>>((acc, replacement, index) => {
        acc[`$${index + 1}`] = replacement
        return acc
      }, {})
    : replacementsRaw

  const result: ReplacedRange = {
    children: [],
    start: 0,
    end: text.length,
    key: `` as any,
    hasClosingTag: false
  }

  const getReplacedContent = (range: ReplacedRange): Content | Content[] => {
    const bits: Content[] = []
    const maybePushSlice = (start: number, end: number) => {
      if (start === end) return
      bits.push(text.slice(start, end))
    }

    let consumed = range.start + range.key.length
    for (let i = 0; i < range.children.length; i++) {
      const child = range.children[i]
      maybePushSlice(consumed, child.start)
      bits.push(getReplacedContent(child))
      consumed = child.end
    }
    // If we have a closing tag (i.e. /$1) it is one character longer than the
    // start tag.
    const contentEnd = range.end - range.key.length - (range.hasClosingTag ? 1 : 0)
    maybePushSlice(consumed, contentEnd)

    const childContent = bits.every(c => typeof c === 'string')
      ? bits.join('')
      : React.createElement(React.Fragment, null, ...bits.filter(b => !!b))

    const replacement = replacements[range.key]
    if (!replacement) return childContent

    return typeof replacement === 'function'
      ? replacement(childContent as string)
      : replacement
  }

  const stack = [result]
  const regex = /\/?\$(\d+)/gm

  // Keep track of the keys we've seen, so we can throw an error if a key is
  // missing.
  const seen = new Set<string>()

  for (const match of text.matchAll(regex)) {
    const tag = match[0]
    const isClosingTag = tag.startsWith('/')
    const key = (isClosingTag ? tag.substring(1) : tag) as keyof typeof replacements

    // If we should throw an error for missing replacements, keep track of the
    // keys we've seen.
    if (!options?.noErrorOnMissingReplacement) {
      seen.add(key)
    }

    // We're not going to replace this key, so ignore it.
    if (!replacements[key]) { continue }

    // If this is a closing tag, pop the element off the stack.
    if (isClosingTag) {
      if (stack.at(-1)!.key === key) {
        stack.pop()
      } else {
        throw new Error(`Mismatched closing tag: ${tag} in message "${text}"`)
      }
      continue
    }

    // Check if we have a closing tag for this key.
    let hasClosingTag = true
    let endIndex = text.indexOf("/" + key, match.index + key.length)

    // If we don't have a closing tag, the end tag is the same as the start tag.
    if (endIndex === -1) {
      endIndex = match.index
      hasClosingTag = false
    } else {
      endIndex += 1 // Include the slash
    }

    // Add the key length so it is fully included in the range.
    endIndex += key.length

    const range: ReplacedRange = {
      children: [],
      end: endIndex,
      start: match.index,
      key: key as `$${number}`,
      hasClosingTag
    }
    stack.at(-1)!.children.push(range)

    // If this is not a self-closing tag, push the element onto the stack, so
    // elements inside it can be added as children.
    if (hasClosingTag) {
      stack.push(range)
    }
  }

  // If we should throw an error for missing replacements check to see if they
  // were all present in our text.
  if (!options?.noErrorOnMissingReplacement && seen.size < Object.keys(replacements).length) {
    throw new Error(`Missing replacements (${Object.keys(replacements).filter(key => !seen.has(key)).join(', ')} we not found in ${text})`)
  }

  const formatted = getReplacedContent(result)
  return (Array.isArray(formatted) ?
    React.createElement(React.Fragment, null, ...formatted)
    : formatted) as ReturnType<T>
}
