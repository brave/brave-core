/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// `*` (zero or more characters) is filled in with a whole placeholder word;
// `?` (per `condition_matcher_util.h`'s Pattern Matcher, exactly one
// character) gets a single placeholder character instead.
const METASYNTACTIC_WORDS = [
  'foo', 'bar', 'baz', 'qux', 'quux', 'corge', 'grault', 'garply', 'waldo',
  'fred', 'plugh', 'xyzzy', 'thud',
]
const SINGLE_CHARACTER_WILDCARD = '?'
const SINGLE_CHARACTER_PLACEHOLDER = 'x'

// Replaces every wildcard in `pattern` with a placeholder so the result is a
// concrete, pasteable example instead of a pattern; cycles through
// `METASYNTACTIC_WORDS` if there are more `*`s than words.
function fillInWildcards(pattern: string, chars: string[]) {
  const regex = new RegExp(`[${chars.map((char) => `\\${char}`).join('')}]`, 'g')
  let wordIndex = 0
  return pattern.replace(regex, (match) => {
    if (match === SINGLE_CHARACTER_WILDCARD) {
      return SINGLE_CHARACTER_PLACEHOLDER
    }
    const word = METASYNTACTIC_WORDS[wordIndex % METASYNTACTIC_WORDS.length]
    wordIndex += 1
    return word
  })
}

// Highlights wildcard characters within `pattern` so they're easy to pick out
// from the literal text around them; shared by conversion URL patterns
// (`*` only, per `MatchUrlPattern` in url_util.cc) and condition matcher
// patterns (`*` and `?`, per `condition_matcher_util.h`'s Pattern Matcher).
// `onCopy` (optional) makes the whole pattern click-to-copy, keeping the
// wildcard highlighting intact; Cmd/Ctrl+click copies a filled-in example
// instead of the raw pattern, e.g. to paste straight into a browser address
// bar.
export function WildcardPattern({ pattern, chars, onCopy }: {
  pattern: string
  chars: string[]
  onCopy?: (value: string) => void
}) {
  const regex = new RegExp(
    `(${chars.map((char) => `\\${char}`).join('|')})`,
  )
  const parts = pattern.split(regex).map((part, index) => (
    chars.includes(part)
      ? <span key={index} className='url-pattern-wildcard'>{part}</span>
      : part
  ))

  if (!onCopy) {
    return <>{parts}</>
  }

  return (
    <span
      className='copyable-text'
      title='(Click to copy; Cmd/Ctrl+click to copy a filled-in example)'
      onClick={(event) => {
        onCopy(
          event.metaKey || event.ctrlKey
            ? fillInWildcards(pattern, chars)
            : pattern,
        )
      }}
    >
      {parts}
    </span>
  )
}
