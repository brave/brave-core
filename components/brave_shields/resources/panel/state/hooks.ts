import * as React from 'react'
import getPanelBrowserAPI, { UIHandlerReceiver, SiteBlockInfo, AdBlockMode } from '../api/panel_browser_api'

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

export function useAdBlockData () {
  const [mode, setMode] = React.useState<AdBlockMode>()

  const getAdBlockMode = async () => {
    const response = await getPanelBrowserAPI().dataHandler.getAdBlockMode()
    setMode(response.mode)
  }

  const handleModeChange = async (e: React.FormEvent<HTMLSelectElement>) => {
    const target = e.target as HTMLSelectElement
    await getPanelBrowserAPI().dataHandler.setAdBlockMode(
      parseInt(target.value, 10))
    await getAdBlockMode()
  }

  React.useEffect(() => {
    const onVisibilityChange = () => {
      if (document.visibilityState === 'visible') {
        getAdBlockMode()
      }
    }

    document.addEventListener('visibilitychange', onVisibilityChange)
    onVisibilityChange()

    return () => {
      document.removeEventListener('visibilitychange', onVisibilityChange)
    }
  }, [])

  return {
    mode,
    handleModeChange
  }
}
