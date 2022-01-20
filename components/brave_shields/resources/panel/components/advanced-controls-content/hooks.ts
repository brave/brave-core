import * as React from 'react'

import { getLocale } from '../../../../../common/locale'
import getPanelBrowserAPI, { AdBlockMode } from '../../api/panel_browser_api'

const adControlTypeOptions = [
  { value: AdBlockMode.AGGRESSIVE, text: getLocale('braveShieldsTrackersAndAdsBlockedAgg') },
  { value: AdBlockMode.STANDARD, text: getLocale('braveShieldsTrackersAndAdsBlockedStd') },
  { value: AdBlockMode.ALLOW, text: getLocale('braveShieldsTrackersAndAdsAllowAll') }
]

export function useTrackerOptions () {
  const [adControlType, setAdControlType] = React.useState<AdBlockMode>()

  const getAdControlType = async () => {
    const response = await getPanelBrowserAPI().dataHandler.getAdBlockMode()
    setAdControlType(response.mode)
  }

  const handleAdControlTypeChange = async (e: React.FormEvent<HTMLSelectElement>) => {
    const target = e.target as HTMLSelectElement
    await getPanelBrowserAPI().dataHandler.setAdBlockMode(
      parseInt(target.value, 10))
    await getAdControlType()
  }

  React.useEffect(() => {
    const onVisibilityChange = () => {
      if (document.visibilityState === 'visible') {
        getAdControlType()
      }
    }

    document.addEventListener('visibilitychange', onVisibilityChange)
    onVisibilityChange()

    return () => {
      document.removeEventListener('visibilitychange', onVisibilityChange)
    }
  }, [])

  return {
    adControlType,
    adControlTypeOptions,
    handleAdControlTypeChange
  }
}
