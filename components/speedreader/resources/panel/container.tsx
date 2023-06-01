// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { dataHandler, SiteSettings, ContentViewSettings, TtsSettings, ToolbarColors, Theme, FontSize, FontFamily, PlaybackSpeed, eventsHandler } from './api/browser'
import Toolbar from './components/toolbar'

function Container() {
  const [siteSettings, setSiteSettings] = React.useState<SiteSettings | undefined>()
  const [contentViewSettings, setContentViewSettings] = React.useState<ContentViewSettings | undefined>()
  const [ttsSettings, setTtsSettings] = React.useState<TtsSettings | undefined>()

  React.useEffect(() => {
    dataHandler.getSiteSettings().then(res => setSiteSettings(res.siteSettings))
    dataHandler.getContentViewSettings().then(res => setContentViewSettings(res.viewSettings))
    dataHandler.getTtsSettings().then(res => setTtsSettings(res.ttsSettings))
    dataHandler.observeThemeChange()
    eventsHandler.onSiteSettingsChanged.addListener((settings: SiteSettings) => {
      setSiteSettings(settings)
    })
    eventsHandler.onContentViewSettingsChanged.addListener((settings: ContentViewSettings) => {
      setContentViewSettings(settings)
    })
    eventsHandler.onBrowserThemeChanged.addListener((colors: ToolbarColors) => {
      const toColor = (color: number) => {
        return '#' +
          (color & 0xffffff).toString(16).padStart(6, '0') +
          (color >>> 24).toString(16).padStart(2, '0')
      }

      const style = document.documentElement.style
      style.setProperty('--color-background', toColor(colors.background))
      style.setProperty('--color-foreground', toColor(colors.foreground))
      style.setProperty('--color-border', toColor(colors.border))
    })
  }, [])

  if (!siteSettings || !contentViewSettings || !ttsSettings) {
    return null
  }

  const handleSpeedreaderChange = (speedreaderEnabled: boolean) => {
    const settings = { ...siteSettings, speedreaderEnabled }
    dataHandler.setSiteSettings(settings)
  }

  const handleThemeChange = (theme: Theme) => {
    const settings = { ...contentViewSettings, theme }
    dataHandler.setContentViewSettings(settings)
  }

  const handleFontSizeChange = (fontSize: FontSize) => {
    const settings = { ...contentViewSettings, fontSize }
    dataHandler.setContentViewSettings(settings)
  }

  const handleFontFamilyChange = (fontFamily: FontFamily) => {
    const settings = { ...contentViewSettings, fontFamily }
    dataHandler.setContentViewSettings(settings)
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

  const handleAiChat = () => {
    dataHandler.aiChat()
  }

  return (
    <Toolbar
      siteSettings={siteSettings}
      contentViewSettings={contentViewSettings}
      ttsSettings={ttsSettings}
      onSpeedreaderChange={handleSpeedreaderChange}
      onThemeChange={handleThemeChange}
      onFontSizeChange={handleFontSizeChange}
      onFontFamilyChange={handleFontFamilyChange}
      onTtsVoiceChange={handleTtsVoiceChange}
      onTtsSpeedChange={handleTtsSpeedChange}
      onAiChat={handleAiChat}
    />
  )
}

export default Container
