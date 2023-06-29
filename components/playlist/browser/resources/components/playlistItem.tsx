/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styled, { css } from 'styled-components'

import { color, font  } from '@brave/leo/tokens/css'
import Icon from '@brave/leo/react/icon'
import DefaultThumbnailIcon from '../assets/playlist-thumbnail-icon.svg'

interface Props {
  id: string
  name: string
  thumbnailUrl: string
  cached: boolean

  onClick: (id: string) => void
}

const ThumbnailStyle = css`
  flex: 0 0 140px;
  height: 80px;
  object-fit: cover;
  border-radius: 4px;
`
const StyledThumbnail = styled.img`
  ${ ThumbnailStyle }
`

const DefaultThumbnail = styled.div`
  ${ ThumbnailStyle }
  content: url(${DefaultThumbnailIcon});
`

const PlaylistContainer = styled.li`
  display: flex;
  padding: 8px 16px 8px 8px;
  align-items: center;
  gap: 16px;
  align-self: stretch;
`

const ItemInfoContainer = styled.div`
  display: flex;
  flex-direction: column;
  gap: 4px;
`

const ItemName = styled.span`
  color: ${color.text.primary};
  font: ${font.desktop.primary.default.semibold};
  cursor: default;
`

const ItemDetailsContainer = styled.div`
  display: flex;
  align-items: center;
  gap: 8px;
  align-self: stretch;
  color: ${color.text.tertiary};
`

export default function PlaylistItem ({
  id,
  name,
  thumbnailUrl,
  cached,
  onClick
}: Props) {
  return (
    <PlaylistContainer className='playlist-item' onClick={() => onClick(id)}>
      <a href={`#${id}`} />
      {thumbnailUrl ? (
        <StyledThumbnail
          className='playlist-item-thumbnail'
          data-id={id}
          src={thumbnailUrl}
        />
      ) : (
        <DefaultThumbnail data-id={id} />
      )}
      <ItemInfoContainer>
        <ItemName className='playlist-item-name'>{name}</ItemName>
        {
          <ItemDetailsContainer>
            {false && <span>duration</span> /* TODO(sko) not ready yet */}
            {cached && <Icon name="check-circle-outline" />}
            {false && <span>350 MB</span> /* TODO(sko) not ready yet */}
          </ItemDetailsContainer>
        }
      </ItemInfoContainer>
    </PlaylistContainer>
  )
}
