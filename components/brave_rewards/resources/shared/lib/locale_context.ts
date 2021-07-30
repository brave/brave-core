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
  const slots: [string, number][] = []

  for (const match of message.matchAll(/([\s\S]*?)(\$\d|$)/g)) {
    if (match[0]) {
      parts.push(match[1] || '')
    }
    if (match[2]) {
      slots.push([match[2], parts.length])
      parts.push(match[2])
    }
  }

  return { parts, slots }
}

interface FormatOptions {
  placeholders?: Record<string, any>
  tags?: Record<string, (content: any) => any>
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
export function formatMessage (
  message: string,
  options: FormatOptions | any[]
) {
  const { parts, slots } = splitMessage(message)

  if (Array.isArray(options)) {
    const placeholders: Record<string, any> = {}
    for (let i = 0; i < options.length; ++i) {
      placeholders[`$${i + 1}`] = options[i]
    }
    options = { placeholders }
  }

  for (const [key, index] of slots) {
    if (options.placeholders && key in options.placeholders) {
      parts[index] = options.placeholders[key]
    }

    if (options.tags && key in options.tags) {
      if (index < parts.length - 1) {
        parts[index] = ''
        parts[index + 1] = options.tags[key](parts[index + 1])
        if (index < parts.length - 2) {
          parts[index + 2] = ''
        }
      }
    }
  }

  return parts
}
