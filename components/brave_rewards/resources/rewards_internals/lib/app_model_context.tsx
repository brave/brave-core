/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { AppState } from './app_state'
import { AppActions } from './app_actions'
import { AppModel } from './app_model'
import { Locale } from './locale_strings'

export const AppModelContext = React.createContext<AppModel | null>(null)

function useAppModel() {
  const model = React.useContext(AppModelContext)
  if (!model) {
    throw new Error('AppModelContext has not been set')
  }
  return model
}

export function useAppState<T>(map: (state: AppState) => T): T {
  const model = useAppModel()
  const [value, setValue] = React.useState(() => map(model.getState()))
  React.useEffect(() => {
    return model.addListener((state) => {
      setValue(map(state))
    })
  }, [model])
  return value
}

export function useAppActions(): AppActions {
  return useAppModel()
}

export function useLocale(): Locale {
  return useAppModel()
}
