/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

export { default as formatMessage } from '$web-common/formatMessage'

export interface Locale<K extends string = string> {
  getString: (key: K) => string
  getPluralString: (key: K, count: number) => Promise<string>
}

export const LocaleContext = React.createContext<Locale>({
  getString: () => '',
  getPluralString: async () => ''
})

// Creates a LocaleContext for testing in storybook, using a dictionary of
// strings.
export function createLocaleContextForTesting (strings: any) {
  const getString =
    (key: string) => String(strings && strings[key] || 'MISSING')
  return {
    getString,
    async getPluralString (key: string, count: number) {
      return getString(key).replace('#', String(count))
    }
  }
}
