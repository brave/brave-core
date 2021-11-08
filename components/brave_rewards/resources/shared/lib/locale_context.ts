/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

export interface Locale {
  getString: (key: string) => string
}

export const LocaleContext = React.createContext<Locale>({
  getString: () => ''
})

function splitMessage (message: string) {
  const parts: any[] = []
  const slots: Array<[string, number]> = []

  for (const match of message.matchAll(/([\s\S]*?)(\$[\d\$]|$)/g)) {
    if (match[1]) {
      parts.push(match[1])
    }
    if (match[2]) {
      slots.push([match[2], parts.length])
      parts.push(match[2])
    }
  }

  return { parts, slots }
}

type TagReplacer =
  ((content: any) => any) |
  { end: string, replace: (content: any) => any }

interface FormatOptions {
  placeholders?: Record<string, any>
  tags?: Record<string, TagReplacer>
}

function createTagStack () {
  const stack: Array<[TagReplacer, number]> = []

  return {
    push (replacer: TagReplacer, start: number) {
      stack.push([replacer, start])
    },

    matches (tag: string) {
      if (stack.length === 0) {
        return false
      }
      const [replacer] = stack[stack.length - 1]
      return typeof replacer !== 'object' || replacer.end === tag
    },

    pop () {
      return stack.pop() || null
    }
  }
}

// Formats a locale-specific message template that contains $N placeholders and
// returns an array containing the processed message parts.
//
// If |options| is an array, then the values in the array are used to fill
// message placeholders, with the first value in the array filling the "$1"
// placeholder, the second value filling the "$2" placeholder, etc.
//
// Example:
//   const result = formatMessage('Your balance is $1 $2, [1, 'BAT'])
//   console.assert(result.join(''), 'Your balance is 1 BAT')
//
// If |options| is an object, then it may contain a |placeholders| property and
// a |tags| property. Placeholders are filled using the |placeholders| object
// and placeholder tag pairs are replaced using the |tags| object.
//
// Example:
//   const result = formatMessage('Hello $1, view $2your profile$3', {
//     placeholders: {
//       $1: 'world'
//     },
//     tags: {
//       $2: (content) => <a key='profile' href='/profile'>{content}</a>
//     }
//   })
//
// Replacements can be made within tags by supplying a |TagReplacer| object in
// the |tags| object. The tag replacer must specify the $N pattern that
// represents the tag ending.
//
// Example:
//   const result = formatMessage('Up to $1$2 devices$3 can be linked', {
//     placeholders: {
//       $2: '4',
//     },
//     tags: {
//       $1: {
//         end: '$3',
//         replace: (content) => <b key='bold'>{content}</b>
//     }
//   });
export function formatMessage (
  message: string,
  options: FormatOptions | any[]
) {
  const { parts, slots } = splitMessage(message)
  const tagStack = createTagStack()

  if (Array.isArray(options)) {
    const placeholders: Record<string, any> = {}
    for (let i = 0; i < options.length; ++i) {
      placeholders[`$${i + 1}`] = options[i]
    }
    options = { placeholders }
  }

  for (const [key, index] of slots) {
    if (key === '$$') {
      parts[index] = '$'
    } else if (tagStack.matches(key)) {
      const top = tagStack.pop()
      if (top) {
        let [replacer, start] = top

        parts[start++] = ''

        const slice = parts.slice(start, index)
        const content = slice.length === 1 ? slice[0] : slice
        parts[start++] = typeof replacer === 'object'
          ? replacer.replace(content)
          : replacer(content)

        while (start <= index) {
          parts[start++] = ''
        }
      }
    } else if (options.placeholders && key in options.placeholders) {
      parts[index] = options.placeholders[key]
    } else if (options.tags && key in options.tags) {
      tagStack.push(options.tags[key], index)
    }
  }

  return parts
}
