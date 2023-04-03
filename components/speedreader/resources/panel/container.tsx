// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { dataHandler, SiteSettings, TtsSettings, Theme, FontSize, FontFamily, PlaybackSpeed, eventsHandler } from './api/browser'
import Toolbar from './components/toolbar'

function Container() {
  const [siteSettings, setSiteSettings] = React.useState<SiteSettings | undefined>()
  const [ttsSettings, setTtsSettings] = React.useState<TtsSettings | undefined>()

  React.useEffect(() => {
    dataHandler.getSiteSettings().then(res => setSiteSettings(res.siteSettings))
    dataHandler.getTtsSettings().then(res => setTtsSettings(res.ttsSettings))
    eventsHandler.onSiteSettingsChanged.addListener((settings: SiteSettings) => {
      setSiteSettings(settings)
    })
  }, [])

  if (!siteSettings || !ttsSettings) {
    return null
  }

  const handleThemeChange = (theme: Theme) => {
    const settings = { ...siteSettings, theme }
    dataHandler.setSiteSettings(settings)
  }

  const handleFontSizeChange = (fontSize: FontSize) => {
    const settings = { ...siteSettings, fontSize }
    dataHandler.setSiteSettings(settings)
  }

  const handleFontFamilyChange = (fontFamily: FontFamily) => {
    const settings = { ...siteSettings, fontFamily }
    dataHandler.setSiteSettings(settings)
  }

  const handleToggleChange = (isEnabled: boolean) => {
    const settings = { ...siteSettings, isEnabled }
    dataHandler.setSiteSettings(settings)
  }

  const handleTtsVoiceChange = (voice: string) => {
    const settings = { ...ttsSettings, voice }
    setTtsSettings(settings)
    dataHandler.setTtsSettings(settings)
  }

  const handleTtsSpeedChange = (speed: PlaybackSpeed) => {
    const settings = { ...ttsSettings, speed }
    setTtsSettings(settings)
    dataHandler.setTtsSettings(settings)
  }

  return (
    <Toolbar
      siteSettings={siteSettings}
      ttsSettings={ttsSettings}
      onThemeChange={handleThemeChange}
      onFontSizeChange={handleFontSizeChange}
      onFontFamilyChange={handleFontFamilyChange}
      onToggleChange={handleToggleChange}
      onTtsVoiceChange={handleTtsVoiceChange}
      onTtsSpeedChange={handleTtsSpeedChange}
    />
  )
}

export default Container
