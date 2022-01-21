import * as React from 'react'

import { SiteBlockInfo, SiteSettings } from '../api/panel_browser_api'

interface Store {
  siteSettings?: SiteSettings
  siteBlockInfo?: SiteBlockInfo
  getSiteSettings?: Function
}

const defaultStore = {
  siteSettings: undefined,
  siteBlockInfo: undefined,
  getSiteSettings: undefined
}

const DataContext = React.createContext<Store>(defaultStore)

export default DataContext
