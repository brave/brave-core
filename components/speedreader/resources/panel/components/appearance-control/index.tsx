// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import * as S from './style'
import ThemeControl from '../theme-control'

import { AppearanceSettings, Theme, FontSize, FontFamily, ColumnWidth } from '../../api/browser'
import { FontStyleList, FontSizeList, ColumnWidthList } from '../lists'
import { getLocale } from '$web-common/locale'

interface AppearanceControlProps {
  appearanceSettings: AppearanceSettings
  onThemeChange: (theme: Theme) => void
  onFontFamilyChange: (fontFamily: FontFamily) => void
  onColumnWidthChange: (columnWidth: ColumnWidth) => void
  onFontSizeChange: (fontSize: FontSize) => void
  onClose: () => void
}

function AppearanceControl (props: AppearanceControlProps) {
  return (
    <S.Box>
      <ThemeControl
        activeTheme={props.appearanceSettings.theme}
        onClick={props.onThemeChange}
      />
      <FontStyleList
        activeFontFamily={props.appearanceSettings.fontFamily}
        onClick={props.onFontFamilyChange}
      />
      <ColumnWidthList
        columnWidth={props.appearanceSettings.columnWidth}
        onClick={props.onColumnWidthChange}
      />
      <FontSizeList
        currentSize={props.appearanceSettings.fontSize}
        onClick={props.onFontSizeChange}
      />
      <S.CloseButton onClick={props.onClose}>
        {getLocale('braveReaderModeClose')}
      </S.CloseButton>
    </S.Box>
  )
}

export default AppearanceControl
