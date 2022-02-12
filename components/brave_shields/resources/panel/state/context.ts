import * as React from 'react'

import { SiteBlockInfo, SiteSettings } from '../api/panel_browser_api'
import { ViewType } from './component_types'

interface Store {
  siteSettings?: SiteSettings
  siteBlockInfo?: SiteBlockInfo
  getSiteSettings?: Function
  setViewType?: ((viewType: ViewType) => void)
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
