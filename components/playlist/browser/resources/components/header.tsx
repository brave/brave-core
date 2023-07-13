// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import styled from 'styled-components'
import { Link } from 'react-router-dom'

import Icon from '@brave/leo/react/icon'
import { color, font, spacing } from '@brave/leo/tokens/css'

import PlaylistInfo from './playlistInfo'
import { usePlaylist } from '../reducers/states'

const StyledLink = styled(Link)`
  text-decoration: none;
  color: unset;
`

interface HeaderProps {
  playlistId?: string
}

const GradientIcon = styled(Icon)`
  --leo-icon-color: linear-gradient(
    314.42deg,
    #fa7250 8.49%,
    #ff1893 43.72%,
    #a78aff 99.51%
  );
`

const ColoredIcon = styled(Icon)<{ color: string }>`
  color: ${p => p.color};
`

const ProductNameContainer = styled.div`
  display: flex;
  gap: 4px;
  font: ${font.primary.heading.h4};
`

const ColoredSpan = styled.span<{ color: string }>`
  color: ${p => p.color};
`

const HeaderContainer = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  padding: ${spacing[16]};
  gap: ${spacing[16]};
  border-bottom: 1px solid ${color.divider.subtle};
  background: ${color.container.background};
`

export default function Header ({ playlistId }: HeaderProps) {
  const playlist = usePlaylist(playlistId)
  return (
    <HeaderContainer>
      {playlist ? (
        <>
          <StyledLink to='/'>
            <ColoredIcon name='arrow-left' color={color.icon.default} />
          </StyledLink>
          <PlaylistInfo
            isDefaultPlaylist={playlist.id === 'default'}
            itemCount={playlist.items.length}
            playlistName={playlist.name}
            totalDuration={0}
            nameColor={color.text.primary}
            detailColor={color.text.secondary}
          />
        </>
      ) : (
        <>
          <GradientIcon name='product-playlist-bold-add-color' />
          <ProductNameContainer>
            <ColoredSpan color={color.text.secondary}>Brave</ColoredSpan>
            <ColoredSpan color={color.text.primary}>Playlist</ColoredSpan>
          </ProductNameContainer>
        </>
      )}
    </HeaderContainer>
  )
}
