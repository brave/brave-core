// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { font } from '@brave/leo/tokens/css/variables'
import styled, { css } from 'styled-components'
import { formatTimeInSeconds } from '../utils/timeFormatter'

interface Props {
  isDefaultPlaylist: boolean
  playlistName: string
  totalDuration?: number
  totalSize: string
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
  justify-content: start;
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
  font: ${font.large.semibold};
  text-overflow: ellipsis;
  overflow: hidden;
  white-space: nowrap;
`

export const PlaylistDetail = styled.div<{ color?: string }>`
  grid-row: 2;
  font: ${font.small.regular};
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
  totalSize,
  nameColor,
  detailColor
}: Props) {
  return (
    <PlaylistInfoContainer className={className}>
      <PlaylistName color={nameColor}>
        {isDefaultPlaylist ? 'Play Later' : playlistName}{' '}
      </PlaylistName>
      <PlaylistDetail color={detailColor}>{itemCount} items</PlaylistDetail>
      {totalDuration !== undefined && (
        <PlaylistDetail color={detailColor}>
          {formatTimeInSeconds(totalDuration, 'space')}
        </PlaylistDetail>
      )}
      {totalSize && (
        <PlaylistDetail color={detailColor}>{totalSize}</PlaylistDetail>
      )}
    </PlaylistInfoContainer>
  )
}
