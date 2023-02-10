// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

import { SiteBlockInfo, SiteSettings } from '../api/panel_browser_api'
import { ViewType } from './component_types'

interface Store {
  siteSettings?: SiteSettings
  siteBlockInfo?: SiteBlockInfo
  getSiteSettings?: Function
  setViewType?: (viewType: ViewType) => void
  viewType: ViewType
}

const defaultStore = {
  siteSettings: undefined,
  siteBlockInfo: undefined,
  getSiteSettings: undefined,
  viewType: ViewType.Main,
  setViewType: undefined
}

const DataContext = React.createContext<Store>(defaultStore)

export default DataContext
