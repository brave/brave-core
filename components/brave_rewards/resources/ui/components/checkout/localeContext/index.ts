/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

export interface LocaleData {
  get: (key: string) => string
}

export const LocaleContext = React.createContext<LocaleData>({
  get (key: string) {
    return 'MISSING'
  }
})
