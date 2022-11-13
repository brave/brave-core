// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { ViewType, Scenes } from './component_types'
import { BrowserProfile } from '../api/import_data_browser'

interface Store {
  setViewType: (viewType: ViewType) => void
  setCurrentSelectedBrowser: (currentSelection: string) => void
  incrementCount: () => void
  setScenes: React.Dispatch<React.SetStateAction<Scenes | undefined>>
  currentSelectedBrowser: string | undefined
  browserProfiles: BrowserProfile[] | undefined
  viewType: ViewType
  scenes: Scenes | undefined
}

const defaultStore = {
  setViewType: () => {},
  setCurrentSelectedBrowser: () => {},
  incrementCount: () => {},
  setScenes: () => {},
  currentSelectedBrowser: undefined,
  browserProfiles: undefined,
  viewType: ViewType.Default,
  scenes: undefined
}

const DataContext = React.createContext<Store>(defaultStore)

export default DataContext
