/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

export interface LocaleInterface<StringKey, PluralStringKey = never> {
  getString: (key: StringKey) => string
  getPluralString: (key: PluralStringKey, count: number) => Promise<string>
}

export function createLocaleContext<StringKey, PluralStringKey>() {
  type Locale = LocaleInterface<StringKey, PluralStringKey>

  const context = React.createContext<Locale>({
    getString: () => '',
    getPluralString: async () => ''
  })

  interface ProviderProps {
    value: Locale
    children: React.ReactNode
  }

  function LocaleProvider(props: ProviderProps) {
    return React.createElement(
      context.Provider, { value: props.value }, props.children)
  }

  function useLocale(): Locale {
    return React.useContext(context)
  }

  function usePluralString(
    key: PluralStringKey,
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

  return { LocaleProvider, useLocale, usePluralString }
}
