// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import getPanelBrowserAPI, { UIHandlerReceiver, SiteBlockInfo, SiteSettings } from '../api/panel_browser_api'
import { loadTimeData } from '../../../../common/loadTimeData'

export function useSiteBlockInfoData () {
  const [siteBlockInfo, setSiteBlockInfo] = React.useState<SiteBlockInfo>()

  React.useEffect(() => {
    const uiHandlerReceiver = new UIHandlerReceiver({
      onSiteBlockInfoChanged: (siteBlockInfo) => {
        setSiteBlockInfo(siteBlockInfo)
      }
    })

    getPanelBrowserAPI().dataHandler.registerUIHandler(
      uiHandlerReceiver.$.bindNewPipeAndPassRemote())
  }, [])

  return { siteBlockInfo }
}

export function useSiteSettingsData () {
  const [siteSettings, setSiteSettings] = React.useState<SiteSettings>()

  const getSiteSettings = async () => {
    const response = await getPanelBrowserAPI().dataHandler.getSiteSettings()
    setSiteSettings(response.siteSettings)
  }

  React.useEffect(() => {
    const onVisibilityChange = () => {
      if (document.visibilityState === 'visible') {
        getSiteSettings()
      }
    }

    document.addEventListener('visibilitychange', onVisibilityChange)
    onVisibilityChange()

    return () => {
      document.removeEventListener('visibilitychange', onVisibilityChange)
    }
  }, [])

  return { siteSettings, getSiteSettings }
}

export function useIsExpanded () {
  const [isExpanded, setIsExpanded] = React.useState<boolean | null>(loadTimeData.getBoolean('isAdvancedViewEnabled'))

  const toggleIsExpanded = () => {
    const newValue = !isExpanded
    setIsExpanded(newValue)
    getPanelBrowserAPI().panelHandler.setAdvancedViewEnabled(newValue)
  }

  React.useEffect(() => {
    const onVisibilityChange = () => {
      if (document.visibilityState === 'visible') {
        getPanelBrowserAPI().panelHandler.getAdvancedViewEnabled().then(res => setIsExpanded(res.isEnabled))
      }
    }

    document.addEventListener('visibilitychange', onVisibilityChange)
    onVisibilityChange()

    return () => {
      document.removeEventListener('visibilitychange', onVisibilityChange)
    }
  }, [])

  return { isExpanded, toggleIsExpanded }
}
