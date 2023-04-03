// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import * as S from './style'
import ThemeControl from '../theme-control'

import { SiteSettings, Theme, FontSize, FontFamily } from '../../api/browser'
import { FontStyleList, FontSizeList } from '../lists'
import { getLocale } from '$web-common/locale'

interface OptionsControlProps {
  siteSettings: SiteSettings
  onThemeChange: (theme: Theme) => void
  onFontFamilyChange: (fontFamily: FontFamily) => void
  onFontSizeChange: (fontSize: FontSize) => void
  onClose: () => void
}

function OptionsControl(props: OptionsControlProps) {
  return (
    <S.Box>
      <ThemeControl
        activeTheme={props.siteSettings.theme}
        onClick={props.onThemeChange}
      />
      <S.VDelemiter />
      <FontStyleList
        activeFontFamily={props.siteSettings.fontFamily}
        onClick={props.onFontFamilyChange}
      />
      <S.VDelemiter />
      <FontSizeList
        currentSize={props.siteSettings.fontSize}
        onClick={props.onFontSizeChange}
      />
      <S.VDelemiter />
      <S.Close onClick={props.onClose}>{getLocale('braveReaderModeClose')}</S.Close>
    </S.Box>
  )
}

export default OptionsControl
