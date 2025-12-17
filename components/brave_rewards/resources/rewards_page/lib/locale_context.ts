/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useAppActions } from './app_context'
import { StringKey } from './locale_strings'

export interface Locale {
  getString: (key: StringKey) => string
}

const LocaleContext = React.createContext<Locale | null>(null)

export const LocaleProvider = LocaleContext.Provider

export function useLocale(): Locale {
  const locale = React.useContext(LocaleContext)
  return locale ?? { getString: () => '' }
}

export function usePluralString(key: StringKey, count: number | undefined) {
  const actions = useAppActions()
  const [value, setValue] = React.useState('')

  React.useEffect(() => {
    if (typeof count !== 'number') {
      setValue('')
      return
    }
    let canUpdate = true
    actions.getPluralString(key, count).then((newValue) => {
      if (canUpdate) {
        setValue(newValue)
      }
    })
    return () => {
      canUpdate = false
    }
  }, [actions, count])

  return value
}
