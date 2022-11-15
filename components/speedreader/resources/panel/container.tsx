// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { panelDataHandler, SiteSettings, Theme, FontSize, ContentStyle, FontFamily } from './api/browser'
import MainPanel from './components/main-panel'

function Container () {
  const [siteSettings, setSiteSettings] = React.useState<SiteSettings | undefined>()

  React.useEffect(() => {
    const onVisibilityChange = () => {
      if (document.visibilityState === 'visible') {
        panelDataHandler.getSiteSettings().then(res => setSiteSettings(res.siteSettings))
      }
    }

    document.addEventListener('visibilitychange', onVisibilityChange)
    panelDataHandler.getSiteSettings().then(res => setSiteSettings(res.siteSettings))

    return () => {
      document.removeEventListener('visibilitychange', onVisibilityChange)
    }
  }, [])

  if (!siteSettings) {
    return null
  }

  const handleThemeChange = (theme: Theme) => {
    setSiteSettings({ ...siteSettings, theme })
    panelDataHandler.setTheme(theme)
  }

  const handleFontSizeChange = (fontSize: FontSize) => {
    setSiteSettings({ ...siteSettings, fontSize })
    panelDataHandler.setFontSize(fontSize)
  }

  const handleContentStyleChange = (contentStyle: ContentStyle) => {
    setSiteSettings({ ...siteSettings, contentStyle })
    panelDataHandler.setContentStyle(contentStyle)
  }

  const handleFontFamilyChange = (fontFamily: FontFamily) => {
    setSiteSettings({ ...siteSettings, fontFamily })
    panelDataHandler.setFontFamily(fontFamily)
  }

  const handleToggleChange = (isOn: boolean) => {
    panelDataHandler.setEnabled(isOn)
    panelDataHandler.getSiteSettings().then(res => setSiteSettings(res.siteSettings))
  }

  return (
   <MainPanel
      siteSettings={siteSettings}
      onThemeChange={handleThemeChange}
      onFontSizeChange={handleFontSizeChange}
      onContentStyleChange={handleContentStyleChange}
      onFontFamilyChange={handleFontFamilyChange}
      onToggleChange={handleToggleChange}
   />
  )
}

export default Container
