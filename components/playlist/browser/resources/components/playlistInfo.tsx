// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { font } from '@brave/leo/tokens/css'
import styled, { css } from 'styled-components'

interface Props {
  isDefaultPlaylist: boolean
  playlistName: string
  totalDuration: number
  itemCount: number
  className?: string

  nameColor?: string
  detailColor?: string
}

export const PlaylistInfoContainer = styled.div`
  display: grid;
  grid-template-rows: repeat(2, auto);
  grid-template-columns: repeat(3, auto);
  column-gap: 8px;
`

export const PlaylistName = styled.div<{ color?: string }>`
  height: 28px;
  grid-row: 1 / 2;
  grid-column: 1 / 4;
  align-self: flex-end;
  ${p =>
    p.color &&
    css`
      color: ${p.color};
    `};
  font: ${font.primary.large.semibold};
`

export const PlaylistDetail = styled.div<{ color?: string }>`
  grid-row: 2;
  font: ${font.primary.small.regular};
  ${p =>
    p.color &&
    css`
      color: ${p.color};
    `};
`

export default function PlaylistInfo ({
  className,
  isDefaultPlaylist,
  playlistName,
  itemCount,
  totalDuration,
  nameColor,
  detailColor
}: Props) {
  return (
    <PlaylistInfoContainer className={className}>
      <PlaylistName color={nameColor}>
        {isDefaultPlaylist ? 'Play Later' : playlistName}{' '}
      </PlaylistName>
      <PlaylistDetail color={detailColor}>{itemCount} items</PlaylistDetail>
      {!!totalDuration && <PlaylistDetail>{totalDuration}</PlaylistDetail>}
      {
        // TODO(sko) We can't get the file size for now
        // !!itemCount && (
        //   <PlaylistDetail className='playlist-detail'>300 mb</PlaylistDetail>
        // )
      }
    </PlaylistInfoContainer>
  )
}
