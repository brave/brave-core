// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

import * as S from './style'
import Toggle from '$web-components/toggle'
import { FontStyleList, ContentList } from '../lists'
import FontSizeControl from '../font-size-control'
import ThemeControl from '../theme-control'
import { SiteSettings, Theme, FontSize, ContentStyle, FontFamily } from '../../api/browser'
import BackgroundSVG from '../../svg/background'
import { getLocale, splitStringForTag } from '$web-common/locale'

interface MainPanelProps {
  siteSettings: SiteSettings
  onThemeChange: (theme: Theme) => void
  onFontSizeChange: (fontSize: FontSize) => void
  onContentStyleChange: (contentStyle: ContentStyle) => void
  onFontFamilyChange: (fontFamily: FontFamily) => void
  onToggleChange: (isOn: boolean) => void
}

function MainContent (props: MainPanelProps) {
  const braveSpeedreaderOffText = splitStringForTag(getLocale('braveSpeedreaderTurnOffDesc'))

  if (!props.siteSettings.isEnabled) {
    return (
      <>
      <S.Section>
        {braveSpeedreaderOffText.beforeTag}
        <a href="brave://settings/">{braveSpeedreaderOffText.duringTag}</a>
        {braveSpeedreaderOffText.afterTag}
      </S.Section>
      <S.Bg><BackgroundSVG /></S.Bg>
      </>
    )
  }

  return (
    <>
      <S.Section>
        <div className="title">{getLocale('braveSpeedreaderThemeLabel')}</div>
        <ThemeControl
          activeTheme={props.siteSettings.theme}
          onClick={props.onThemeChange}
        />
      </S.Section>
      <S.Section>
        <div className="title">{getLocale('braveSpeedreaderFontStyleLabel')}</div>
        <FontStyleList
          activeFontFamily={props.siteSettings.fontFamily}
          onClick={props.onFontFamilyChange}
        />
        <br />
        <FontSizeControl
          currentSize={props.siteSettings.fontSize}
          onClick={props.onFontSizeChange}
        />
      </S.Section>
      <S.Section>
        <div className="title">{getLocale('braveSpeedreaderContentStyleLabel')}</div>
        <ContentList
          activeContentStyle={props.siteSettings.contentStyle}
          onClick={props.onContentStyleChange}
        />
      </S.Section>
    </>
  )
}

function MainPanel (props: MainPanelProps) {
  return (
    <S.Box>
      <S.HeaderBox>
        <S.HeaderContent>
          <div>{getLocale('braveSpeedreader')}</div>
          <div>
            <Toggle
              brand="shields"
              isOn={props.siteSettings.isEnabled}
              onChange={props.onToggleChange}
            />
          </div>
        </S.HeaderContent>
        {props.siteSettings.isEnabled && (
          <S.SiteTitleBox>
            {getLocale('braveSpeedreaderAlwaysLoadLabel').replace('$1', props.siteSettings.host)}
          </S.SiteTitleBox>
        )}
      </S.HeaderBox>
      <MainContent {...props} />
    </S.Box>
  )
}

export default MainPanel
