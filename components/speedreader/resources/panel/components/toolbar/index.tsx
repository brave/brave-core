// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import * as S from './style'
import { SiteSettings, TtsSettings, Theme, FontSize, FontFamily, PlaybackSpeed, dataHandler } from '../../api/browser'
import { MainButtonType, MainButtonsList } from '../lists'
import ReaderModeControl from "../reader-mode-control"
import OptionsControl from "../options-control"
import TtsControl from '../tts-control'

interface ToolbarProps {
  siteSettings: SiteSettings
  ttsSettings: TtsSettings
  onThemeChange: (theme: Theme) => void
  onFontSizeChange: (fontSize: FontSize) => void
  onFontFamilyChange: (fontFamily: FontFamily) => void
  onToggleChange: (isEnabled: boolean) => void
  onTtsVoiceChange: (voice: string) => void;
  onTtsSpeedChange: (speed: PlaybackSpeed) => void
}

function Toolbar(props: ToolbarProps) {
  const [activeButton, setActiveButton] = React.useState(MainButtonType.None)

  const handleMainButtonClick = (button: MainButtonType) => {
    if (button == MainButtonType.ShowOriginal) {
      dataHandler.viewOriginal()
    } else {
      setActiveButton(button)
    }
  }

  return (
    <S.Box>
      <MainButtonsList
        activeButton={activeButton}
        onClick={handleMainButtonClick.bind(this)}
      />
      {activeButton === MainButtonType.None && (<ReaderModeControl />)}
      {activeButton === MainButtonType.Options && (
        <OptionsControl
          siteSettings={props.siteSettings}
          onThemeChange={props.onThemeChange}
          onFontFamilyChange={props.onFontFamilyChange}
          onFontSizeChange={props.onFontSizeChange}
          onClose={() => { setActiveButton(MainButtonType.None) }}
        />)}
      {activeButton === MainButtonType.TextToSpeech && (
        <TtsControl
          ttsSettings={props.ttsSettings}
          onTtsVoiceChange={props.onTtsVoiceChange}
          onTtsSpeedChange={props.onTtsSpeedChange}
          onClose={() => { setActiveButton(MainButtonType.None) }}
        />
      )}
    </S.Box>
  )
}

export default Toolbar
