// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

import * as S from './style'
import { SiteSettings, Theme, FontSize, ContentStyle, FontFamily } from '../../api/browser'
import { MainButtonType, MainButtonsList } from '../lists'
import ReaderModeControl from "../reader-mode-control"

interface MainPanelProps {
  siteSettings: SiteSettings
  onThemeChange: (theme: Theme) => void
  onFontSizeChange: (fontSize: FontSize) => void
  onContentStyleChange: (contentStyle: ContentStyle) => void
  onFontFamilyChange: (fontFamily: FontFamily) => void
  onToggleChange: (isOn: boolean) => void
}

function MainPanel(props: MainPanelProps) {
  const [activeButton, setActiveButton] = React.useState(MainButtonType.None)

  return (
    <S.Box>
      <MainButtonsList activeButton={activeButton} onClick={setActiveButton} />
      {activeButton === MainButtonType.None && (<ReaderModeControl />)}
      {activeButton === MainButtonType.Options && (<OptionsControl />)}
    </S.Box>
  )
}

export default MainPanel
