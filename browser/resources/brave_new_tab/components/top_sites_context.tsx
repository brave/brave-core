/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { TopSitesModel, TopSitesState, defaultModel } from '../models/top_sites_model'
import { useModelState } from '../lib/use_model_state'

const Context = React.createContext<TopSitesModel>(defaultModel())

interface Props {
  model: TopSitesModel
  children: React.ReactNode
}

export function TopSitesContext(props: Props) {
  return (
    <Context.Provider value={props.model}>
      {props.children}
    </Context.Provider>
  )
}

export function useTopSitesModel(): TopSitesModel {
  return React.useContext(Context)
}

export function useTopSitesState<T>(map: (state: TopSitesState) => T): T {
  return useModelState(useTopSitesModel(), map)
}
