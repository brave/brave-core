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

// Splits |message| into parts using "$N" patterns as delimiters,
// and calls |fn| with each part as a separate argument.
export function formatMessageParts<T> (
  message: string,
  fn: (...parts: string[]) => T
) {
  return fn(...message.split(/\$\d/g))
}
