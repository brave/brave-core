/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { AppModel, AppActions, AppState } from '../../models/app_model'

const Context = React.createContext<AppModel | null>(null)

interface Props {
  model: AppModel
  children: React.ReactNode
}

export function AppModelContext(props: Props) {
  return (
    <Context.Provider value={props.model}>
      {props.children}
    </Context.Provider>
  )
}

function useAppModel(): AppModel {
  const appModel = React.useContext(Context)
  if (!appModel) {
    throw new Error('AppModel context has not been set')
  }
  return appModel
}

export function useAppActions(): AppActions {
  return useAppModel()
}

export function useAppState<T>(map: (state: AppState) => T): T {
  const model = useAppModel()
  const [value, setValue] = React.useState(() => map(model.getState()))
  React.useEffect(() => {
    setValue(map(model.getState()))
    return model.addListener((state) => { setValue(map(state)) })
  }, [model])
  return value
}
