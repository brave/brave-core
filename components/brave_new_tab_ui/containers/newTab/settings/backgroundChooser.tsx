// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import {
  SettingsRow,
  SettingsText,
  StyledCustomBackgroundSettings
} from '../../../components/default'
import NavigateBack from '../../../components/default/settings/navigateBack'
import BackgroundOption from './backgroundOption'
import Toggle from '@brave/leo/react/toggle'

import { getLocale } from '../../../../common/locale'

interface Props {
  title: string
  backgrounds: NewTab.BackgroundWallpaper[]
  currentValue?: string
  usingRandomColor: boolean
  renderExtraButton?: () => JSX.Element
  onSelectValue: (background: string, useRandomColor: boolean) => void
  onBack: () => void
  onToggleRandomColor: (on: boolean) => void
  onRemoveValue?: (background: string) => void
}

function BackgroundChooser ({ title, backgrounds, onBack, onSelectValue, currentValue, usingRandomColor, onToggleRandomColor, renderExtraButton, onRemoveValue }: Props) {
  const containerEl = React.useRef<HTMLDivElement>(null)
  React.useEffect(() => {
    containerEl.current?.scrollIntoView(true)
  }, [])

  return (
      <div ref={containerEl}>
        <NavigateBack onBack={onBack} title={title} />
        <SettingsRow>
          <SettingsText>{getLocale('refreshBackgroundOnNewTab')}</SettingsText>
          <Toggle
            onChange={e => onToggleRandomColor(e.checked)}
            checked={usingRandomColor}
          />
        </SettingsRow>
        <StyledCustomBackgroundSettings>
          {backgrounds.map((background) => {
            const value = background.type === 'color' ? background.wallpaperColor : background.wallpaperImageUrl
            return <BackgroundOption key={value} background={background}
                      selected={!usingRandomColor && currentValue === value}
                      onSelectValue={() => onSelectValue(value, /* useRandomColor= */false)}
                      onRemoveValue={onRemoveValue ? () => { onRemoveValue(value) } : undefined} />
          })}
          { renderExtraButton?.() }
        </StyledCustomBackgroundSettings>
      </div>
  )
}

export default BackgroundChooser
