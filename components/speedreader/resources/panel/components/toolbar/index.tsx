// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import * as S from './style'
import { dataHandler, SiteSettings, AppearanceSettings, TtsSettings, ToolbarColors, Theme, FontSize, FontFamily, PlaybackSpeed, eventsHandler } from '../../api/browser'
import { MainButtonType, MainButtonsList } from '../lists'
import ReaderModeControl from "../reader-mode-control"
import OptionsControl from "../options-control"
import TtsControl from '../tts-control'

const toColor = (color: number) => {
  return '#' +
    (color & 0xffffff).toString(16).padStart(6, '0') +
    (color >>> 24).toString(16).padStart(2, '0')
}

function Toolbar() {
  const [siteSettings, setSiteSettings] = React.useState<SiteSettings | undefined>()
  const [appearanceSettings, setAppearanceSettings] = React.useState<AppearanceSettings | undefined>()
  const [ttsSettings, setTtsSettings] = React.useState<TtsSettings | undefined>()
  const [activeButton, setActiveButton] = React.useState(MainButtonType.None)

  React.useEffect(() => {
    dataHandler.getSiteSettings().then((res: any) => setSiteSettings(res.siteSettings))
    dataHandler.getAppearanceSettings().then((res: any) => setAppearanceSettings(res.appearanceSettings))
    dataHandler.getTtsSettings().then((res: any) => setTtsSettings(res.ttsSettings))
    dataHandler.observeThemeChange()
    eventsHandler.onSiteSettingsChanged.addListener((settings: SiteSettings) => {
      setSiteSettings(settings)
    })
    eventsHandler.onAppearanceSettingsChanged.addListener((settings: AppearanceSettings) => {
      setAppearanceSettings(settings)
    })
    eventsHandler.onBrowserThemeChanged.addListener((colors: ToolbarColors) => {
      const style = document.documentElement.style
      style.setProperty('--color-background', toColor(colors.background))
      style.setProperty('--color-foreground', toColor(colors.foreground))
      style.setProperty('--color-border', toColor(colors.border))
    })
  }, [])

  if (!siteSettings || !appearanceSettings || !ttsSettings) {
    return null
  }

  const handleSpeedreaderChange = (speedreaderEnabled: boolean) => {
    const settings = { ...siteSettings, speedreaderEnabled }
    dataHandler.setSiteSettings(settings)
  }

  const handleThemeChange = (theme: Theme) => {
    const settings = { ...appearanceSettings, theme }
    dataHandler.setAppearanceSettings(settings)
  }

  const handleFontSizeChange = (fontSize: FontSize) => {
    const settings = { ...appearanceSettings, fontSize }
    dataHandler.setAppearanceSettings(settings)
  }

  const handleFontFamilyChange = (fontFamily: FontFamily) => {
    const settings = { ...appearanceSettings, fontFamily }
    dataHandler.setAppearanceSettings(settings)
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

  const handleMainButtonClick = (button: MainButtonType) => {
    if (button === MainButtonType.Speedreader) {
      handleSpeedreaderChange(siteSettings.speedreaderEnabled)
    } else if (button === MainButtonType.AI) {
      handleAiChat()
    } else {
      if (activeButton !== button) {
        setActiveButton(button)
      } else {
        setActiveButton(MainButtonType.None)
      }
    }
  }

  return (
    <S.Box>
      <MainButtonsList
        activeButton={activeButton}
        onClick={handleMainButtonClick.bind(this)}
      />
      {activeButton === MainButtonType.None && (
        <ReaderModeControl siteSettings={siteSettings} />)
      }
      {activeButton === MainButtonType.Options && (
        <OptionsControl
          appearanceSettings={appearanceSettings}
          onThemeChange={handleThemeChange.bind(this)}
          onFontFamilyChange={handleFontFamilyChange.bind(this)}
          onFontSizeChange={handleFontSizeChange.bind(this)}
          onClose={() => { setActiveButton(MainButtonType.None) }}
        />)}
      {activeButton === MainButtonType.TextToSpeech && (
        <TtsControl
          ttsSettings={ttsSettings}
          onTtsVoiceChange={handleTtsVoiceChange.bind(this)}
          onTtsSpeedChange={handleTtsSpeedChange.bind(this)}
          onClose={() => { setActiveButton(MainButtonType.None) }}
        />
      )}
    </S.Box>
  )
}

export default Toolbar
