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

export const getLocaleWithTag = (text: string) => {
  const actionIndex: number = text.indexOf('$1')
  const actionEndIndex: number = text.indexOf('$2')
  const beforeTag = text.substring(0, actionIndex)
  const duringTag = text.substring(actionIndex + 2, actionEndIndex)
  const afterTag = text.substring(actionEndIndex + 2)
  return { beforeTag, duringTag, afterTag }
}
