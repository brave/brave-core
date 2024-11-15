/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { SearchModel, SearchState, defaultModel } from '../models/search_model'
import { useModelState } from '../lib/use_model_state'

const Context = React.createContext<SearchModel>(defaultModel())

interface Props {
  model: SearchModel
  children: React.ReactNode
}

export function SearchContext(props: Props) {
  return (
    <Context.Provider value={props.model}>
      {props.children}
    </Context.Provider>
  )
}

export function useSearchModel(): SearchModel {
  return React.useContext(Context)
}

export function useSearchState<T>(map: (state: SearchState) => T): T {
  return useModelState(useSearchModel(), map)
}
