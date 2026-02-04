/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { StringKey } from './locale_strings'
import { defaultAppStore } from './app_store'
import { createUseStateHook } from './state_store_hooks'

export const AppContext = React.createContext(defaultAppStore())

export const useAppState = createUseStateHook(AppContext)

export function useAppActions() {
  return useAppState((s) => s.actions)
}

export function usePluralString(key: StringKey, count: number | undefined) {
  const { getPluralString } = useAppActions()
  const [value, setValue] = React.useState('')

  React.useEffect(() => {
    if (typeof count !== 'number') {
      setValue('')
      return
    }
    let canUpdate = true
    getPluralString(key, count).then((newValue) => {
      if (canUpdate) {
        setValue(newValue)
      }
    })
    return () => {
      canUpdate = false
    }
  }, [getPluralString, count])

  return value
}
