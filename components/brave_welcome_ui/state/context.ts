// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { ViewType } from './component_types'
import { BrowserProfile } from '../api/import_data_browser'

interface Store {
  setViewType: (viewType: ViewType) => void
  setCurrentSelectedBrowser: (currentSelection: string) => void
  incrementCount: () => void
  currentSelectedBrowser: string | undefined
  browserProfiles: BrowserProfile[] | undefined
  viewType: ViewType
}

const defaultStore = {
  setViewType: () => {},
  setCurrentSelectedBrowser: () => {},
  incrementCount: () => {},
  currentSelectedBrowser: undefined,
  browserProfiles: undefined,
  viewType: ViewType.Default
}

const DataContext = React.createContext<Store>(defaultStore)

export default DataContext
