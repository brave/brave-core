// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'

import {
  StyledCustomBackgroundOption,
  StyledCustomBackgroundOptionColor,
  StyledCustomBackgroundOptionImage,
  StyledSelectionBorder
} from '../../../components/default'

import { CloseCircleIcon } from 'brave-ui/components/icons'

interface Props {
  background: NewTab.BackgroundWallpaper
  selected: boolean
  onSelectValue: (value: NewTab.BackgroundWallpaper) => void
  onRemoveValue?: (value: NewTab.BackgroundWallpaper) => void
}

const StyledRemoveButton = styled.div<{ hovered: boolean }>`
  position: absolute;
  top: 10px;
  right: 10px;
  width: 40px;
  height: 40px;
  border: unset;
  background: transparent;
  color: white;
  visibility: ${p => p.hovered ? 'visible' : 'hidden'};
  & svg {
    border-radius: 20px;
    filter: drop-shadow( 0 0 5px rgba(0, 0, 0, .7));
  }
`

export default function BackgroundOption ({ background, selected, onSelectValue, onRemoveValue }: Props) {
  const [hovered, setHovered] = React.useState(false)
  return (
    <StyledCustomBackgroundOption onClick={_ => onSelectValue(background) } onMouseOver={() => setHovered(true)} onMouseLeave={() => setHovered(false)}>
      <StyledSelectionBorder selected={selected} removable={!!onRemoveValue}>
          { onRemoveValue &&
            <StyledRemoveButton hovered={hovered} onClick={(e) => {
                onRemoveValue(background)
                e.stopPropagation()
            }}>
              <CloseCircleIcon />
            </StyledRemoveButton>
          }
          {
            background.type === 'color'
              ? <StyledCustomBackgroundOptionColor colorValue={background.wallpaperColor} selected={selected}/>
              : <StyledCustomBackgroundOptionImage image={background.wallpaperImageUrl } selected={selected}/>
          }
      </StyledSelectionBorder>
    </StyledCustomBackgroundOption>
  )
}
