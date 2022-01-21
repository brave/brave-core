import * as React from 'react'
import getPanelBrowserAPI, { UIHandlerReceiver, SiteBlockInfo, SiteSettings } from '../api/panel_browser_api'

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
