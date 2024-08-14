// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import * as S from './style'
import getToolbarAPI, { MainButtonType, AppearanceSettings, TtsSettings, ToolbarColors, Theme, FontSize, FontFamily, PlaybackSpeed, PlaybackState, ColumnWidth } from '../../api/browser'
import { MainButtonsList } from '../lists'
import ReaderModeControl from "../reader-mode-control"
import AppearanceControl from "../appearance-control"
import TtsControl from '../tts-control'

const toColor = (color: number) => {
  return '#' +
    (color & 0xffffff).toString(16).padStart(6, '0') +
    (color >>> 24).toString(16).padStart(2, '0')
}

function Toolbar() {
  const [appearanceSettings, setAppearanceSettings] = React.useState<AppearanceSettings | undefined>()
  const [ttsSettings, setTtsSettings] = React.useState<TtsSettings | undefined>()
  const [activeButton, setActiveButton] = React.useState(MainButtonType.None)

  React.useEffect(() => {
    getToolbarAPI().dataHandler.getAppearanceSettings().then((res: any) => setAppearanceSettings(res.appearanceSettings))
    getToolbarAPI().dataHandler.getTtsSettings().then((res: any) => setTtsSettings(res.ttsSettings))
    getToolbarAPI().dataHandler.observeThemeChange()
    getToolbarAPI().eventsRouter.onAppearanceSettingsChanged.addListener((settings: AppearanceSettings) => {
      setAppearanceSettings(settings)
    })
    getToolbarAPI().eventsRouter.setPlaybackState.addListener((state: PlaybackState) => {
      if (state !== PlaybackState.kStopped) {
        setActiveButton(MainButtonType.TextToSpeech)
      }
    })

    getToolbarAPI().eventsRouter.onBrowserThemeChanged.addListener((colors: ToolbarColors) => {
      const style = document.documentElement.style
      style.setProperty('--color-background', toColor(colors.background))
      style.setProperty('--color-foreground', toColor(colors.foreground))
      style.setProperty('--color-border', toColor(colors.border))
      style.setProperty('--color-button-border', toColor(colors.buttonBorder))
      style.setProperty('--color-button-hover', toColor(colors.buttonHover))
      style.setProperty('--color-button-active', toColor(colors.buttonActive))
      style.setProperty('--color-button-active-text', toColor(colors.buttonActiveText))
    })
  }, [])

  React.useEffect(() => {
    getToolbarAPI().dataHandler.onToolbarStateChanged(activeButton)

    const onKeydown = (event: KeyboardEvent) => {
      event.stopPropagation()

      if (event.code === 'Escape' && !event.altKey && !event.ctrlKey && !event.metaKey && !event.shiftKey) {
        if (activeButton === MainButtonType.None) {
          getToolbarAPI().dataHandler.viewOriginal()
        } else {
          setActiveButton(MainButtonType.None)
        }
      }
    }

    const id = getToolbarAPI().eventsRouter.onTuneBubbleClosed.addListener(() => {
      if (activeButton === MainButtonType.Tune) {
        setActiveButton(MainButtonType.None)
      }
    })

    document.body.addEventListener('keydown', onKeydown)
    return () => {
      document.body.removeEventListener('keydown', onKeydown)
      getToolbarAPI().eventsRouter.removeListener(id)
    }
  }, [activeButton])


  if (!appearanceSettings || !ttsSettings) {
    return null
  }

  const handleClose = () => {
    getToolbarAPI().dataHandler.viewOriginal()
  }

  const handleThemeChange = (theme: Theme) => {
    const settings = { ...appearanceSettings, theme }
    getToolbarAPI().dataHandler.setAppearanceSettings(settings)
  }

  const handleFontSizeChange = (fontSize: FontSize) => {
    const settings = { ...appearanceSettings, fontSize }
    getToolbarAPI().dataHandler.setAppearanceSettings(settings)
  }

  const handleFontFamilyChange = (fontFamily: FontFamily) => {
    const settings = { ...appearanceSettings, fontFamily }
    getToolbarAPI().dataHandler.setAppearanceSettings(settings)
  }

  const handleColumnWidthChange = (columnWidth: ColumnWidth) => {
    const settings = { ...appearanceSettings, columnWidth }
    getToolbarAPI().dataHandler.setAppearanceSettings(settings)
  }

  const handleTtsVoiceChange = (voice: string) => {
    const settings = { ...ttsSettings, voice }
    setTtsSettings(settings)
    getToolbarAPI().dataHandler.setTtsSettings(settings)
  }

  const handleTtsSpeedChange = (speed: PlaybackSpeed) => {
    const settings = { ...ttsSettings, speed }
    setTtsSettings(settings)
    getToolbarAPI().dataHandler.setTtsSettings(settings)
  }

  const handleAiChat = () => {
    getToolbarAPI().dataHandler.aiChat()
  }

  const handleTune = (show: boolean) => {
    getToolbarAPI().dataHandler.showTuneBubble(show)
  }

  const handleMainButtonClick = (button: MainButtonType) => {
    if (button === MainButtonType.AI) {
      handleAiChat()
    } else {
      if (activeButton !== button) {
        setActiveButton(button)
      } else {
        setActiveButton(MainButtonType.None)
      }
      if (button === MainButtonType.Tune) {
        handleTune(activeButton !== button)
      }
    }
  }

  return (
    <S.Box>
      <MainButtonsList
        activeButton={activeButton}
        onClick={handleMainButtonClick.bind(this)}
      />
      {(activeButton === MainButtonType.None || activeButton === MainButtonType.Tune) &&
        (<ReaderModeControl onClose={handleClose.bind(this)} />)
      }
      {activeButton === MainButtonType.Appearance && (
        <AppearanceControl
          appearanceSettings={appearanceSettings}
          onThemeChange={handleThemeChange.bind(this)}
          onFontFamilyChange={handleFontFamilyChange.bind(this)}
          onColumnWidthChange={handleColumnWidthChange.bind(this)}
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
