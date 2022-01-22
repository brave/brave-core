import * as React from 'react'

import { AdBlockMode, SiteBlockInfo } from '../api/panel_browser_api'

interface Store {
  adBlock: {
    mode: AdBlockMode | undefined
    handleModeChange: React.ChangeEventHandler<HTMLSelectElement> | undefined } | undefined
  siteBlockInfo: SiteBlockInfo | undefined
}

const defaultStore = {
  adBlock: undefined,
  siteBlockInfo: undefined
}

const DataContext = React.createContext<Store>(defaultStore)

export default DataContext
