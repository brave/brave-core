/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { StringKey } from '../lib/locale_strings'

export interface Locale {
  getString: (key: StringKey) => string
  getPluralString: (key: StringKey, count: number) => Promise<string>
}

const Context = React.createContext<Locale>({
  getString: () => '',
  getPluralString: async () => ''
})

interface Props {
  locale: Locale
  children: React.ReactNode
}

export function LocaleContext(props: Props) {
  return (
    <Context.Provider value={props.locale}>
      {props.children}
    </Context.Provider>
  )
}

export function useLocale(): Locale {
  return React.useContext(Context)
}

export function usePluralString(
  key: StringKey,
  count: number | undefined | null
) {
  const locale = useLocale()
  const [value, setValue] = React.useState('')

  React.useEffect(() => {
    if (typeof count !== 'number') {
      setValue('')
      return
    }
    let canUpdate = true
    locale.getPluralString(key, count).then((newValue) => {
      if (canUpdate) {
        setValue(newValue)
      }
    })
    return () => { canUpdate = false }
  }, [locale, count])

  return value
}
