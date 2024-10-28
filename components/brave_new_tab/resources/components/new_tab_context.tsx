/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { NewTabModel, NewTabState, defaultModel } from '../models/new_tab_model'
import { useModelState } from '../lib/use_model_state'

const Context = React.createContext<NewTabModel>(defaultModel())

interface Props {
  model: NewTabModel
  children: React.ReactNode
}

export function NewTabContext(props: Props) {
  return (
    <Context.Provider value={props.model}>
      {props.children}
    </Context.Provider>
  )
}

export function useNewTabModel(): NewTabModel {
  return React.useContext(Context)
}

export function useNewTabState<T>(map: (state: NewTabState) => T): T {
  return useModelState(useNewTabModel(), map)
}
