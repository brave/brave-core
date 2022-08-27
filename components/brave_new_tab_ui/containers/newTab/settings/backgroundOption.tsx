// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

import {
  StyledCustomBackgroundOption,
  StyledCustomBackgroundOptionColor,
  StyledCustomBackgroundOptionImage,
  StyledSelectionBorder
} from '../../../components/default'

interface Props {
  background: NewTab.BackgroundWallpaper
  selected: boolean
  onSelectValue: (value: NewTab.BackgroundWallpaper) => void
}

export default function BackgroundOption ({ background, selected, onSelectValue }: Props) {
  return (
    <StyledCustomBackgroundOption onClick={_ => onSelectValue(background)}>
      <StyledSelectionBorder selected={selected}>
          {
            background.type === 'color'
              ? <StyledCustomBackgroundOptionColor colorValue={background.wallpaperColor} selected={selected}/>
              : <StyledCustomBackgroundOptionImage image={background.wallpaperImageUrl } selected={selected}/>
          }
      </StyledSelectionBorder>
    </StyledCustomBackgroundOption>
  )
}
