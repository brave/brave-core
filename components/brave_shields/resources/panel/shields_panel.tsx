// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from 'react-dom'
import { initLocale } from 'brave-ui'

import { setIconBasePath } from '@brave/leo/react/icon'

import { loadTimeData } from '$web-common/loadTimeData'
import BraveCoreThemeProvider from '$web-common/BraveCoreThemeProvider'
import { PanelWrapper } from './style'
import getPanelBrowserAPI from './api/panel_browser_api'
import Container from './container'
import { useSiteBlockInfoData, useSiteSettingsData } from './state/hooks'
import DataContext from './state/context'
import { ViewType } from './state/component_types'

setIconBasePath('//resources/brave-icons')

function App () {
  const { siteBlockInfo } = useSiteBlockInfoData()
  const { siteSettings, getSiteSettings } = useSiteSettingsData()
  const [viewType, setViewType] = React.useState<ViewType>(ViewType.Main)

  const store = {
    siteBlockInfo,
    siteSettings,
    getSiteSettings,
    viewType,
    setViewType
  }

  React.useEffect(() => {
    const onVisibilityChange = () => {
      if (document.visibilityState === 'visible') {
        getPanelBrowserAPI().panelHandler.showUI()
        getPanelBrowserAPI().dataHandler.updateFavicon()
        setViewType(ViewType.Main) /* Reset the view back to main panel */
      }
    }

    document.addEventListener('visibilitychange', onVisibilityChange)

    return () => {
      document.removeEventListener('visibilitychange', onVisibilityChange)
    }
  }, [])

  return (<DataContext.Provider
    value={store}
  >
    <BraveCoreThemeProvider>
      <PanelWrapper>
        <Container />
      </PanelWrapper>
    </BraveCoreThemeProvider>
  </DataContext.Provider>)
}

function initialize () {
  initLocale(loadTimeData.data_)
  render(<App />, document.getElementById('mountPoint'),
  () => {
    getPanelBrowserAPI().panelHandler.showUI()
  })
}

document.addEventListener('DOMContentLoaded', initialize)
