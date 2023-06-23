// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import styled from 'styled-components'
import { Link } from 'react-router-dom'
import { useSelector } from 'react-redux'

import Icon from '@brave/leo/react/icon'
import { color, font, spacing } from '@brave/leo/tokens/css'

import * as PlaylistMojo from 'gen/brave/components/playlist/common/mojom/playlist.mojom.m.js'
import PlaylistInfo from './playlistInfo'
import * as States from '../reducers/states'

const StyledLink = styled(Link)`
  text-decoration: none;
  color: unset;
`

interface HeaderProps {
  playlistId?: String
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
  font: ${font.desktop.heading.h4};
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
  height: 100%;
  border-bottom: 1px solid ${color.divider.subtle};
  background: ${color.container.background};
`

export default function Header ({ playlistId }: HeaderProps) {
  const playlist = useSelector<
    States.ApplicationState,
    PlaylistMojo.Playlist | undefined
  >(applicationState =>
    applicationState.playlistData?.lists.find(e => e.id === playlistId)
  )

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
          />
        </>
      ) : (
        <>
          <GradientIcon name='product-playlist-bold-add' />
          <ProductNameContainer>
            <ColoredSpan color={color.text.secondary}>Brave</ColoredSpan>
            <ColoredSpan color={color.text.primary}>Playlist</ColoredSpan>
          </ProductNameContainer>
        </>
      )}
    </HeaderContainer>
  )
}
