/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styled, { css } from 'styled-components'

import { color, font } from '@brave/leo/tokens/css'
import Icon from '@brave/leo/react/icon'
import DefaultThumbnailIcon from '../assets/playlist-thumbnail-icon.svg'
import ContextualMenuAnchorButton from './contextualMenu'
import { PlaylistItem as PlaylistItemMojo } from 'gen/brave/components/playlist/common/mojom/playlist.mojom.m.js'
import {
  DelimType,
  formatTimeInSeconds,
  toSeconds
} from '../utils/timeFormatter'

interface Props {
  item: PlaylistItemMojo

  onClick: (id: string) => void
}

const ThumbnailStyle = css`
  flex: 0 0 140px;
  height: 80px;
  object-fit: cover;
  border-radius: 4px;
`
const StyledThumbnail = styled.img`
  ${ThumbnailStyle}
`

const DefaultThumbnail = styled.div`
  ${ThumbnailStyle}
  content: url(${DefaultThumbnailIcon});
`

const PlaylistItemContainer = styled.li`
  display: flex;
  padding: 8px 16px 8px 8px;
  align-items: center;
  gap: 16px;
  align-self: stretch;
`

const ItemInfoContainer = styled.div`
  flex-grow: 1;
  display: flex;
  flex-direction: column;
  gap: 4px;
`

const PlaylistItemName = styled.span`
  color: ${color.text.primary};
  font: ${font.primary.xSmall.regular};
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
  item: {
    id,
    name,
    cached,
    duration,
    thumbnailPath: { url: thumbnailUrl }
  },
  onClick
}: Props) {
  const anchorElem = React.useRef<HTMLAnchorElement>(null)

  const scrollToThisIfNeeded = React.useCallback(() => {
    if (window.location.hash.replace('#', '') !== id) return

    if (anchorElem.current)
      window.scrollTo({ top: anchorElem.current.offsetTop })
  }, [id])

  React.useEffect(() => {
    window.addEventListener('hashchange', scrollToThisIfNeeded)
    return () => {
      window.removeEventListener('hashchange', scrollToThisIfNeeded)
    }
  }, [scrollToThisIfNeeded])

  React.useEffect(() => scrollToThisIfNeeded(), [])

  const [hovered, setHovered] = React.useState(false)
  const [showingMenu, setShowingMenu] = React.useState(false)

  return (
    <PlaylistItemContainer
      onClick={() => onClick(id)}
      onMouseEnter={() => setHovered(true)}
      onMouseLeave={e => setHovered(false)}
    >
      <a ref={anchorElem} href={`#${id}`} />
      {thumbnailUrl ? (
        <StyledThumbnail src={thumbnailUrl} />
      ) : (
        <DefaultThumbnail />
      )}
      <ItemInfoContainer>
        <PlaylistItemName>{name}</PlaylistItemName>
        {
          <ItemDetailsContainer>
            {duration && (
              <span>
                {formatTimeInSeconds(toSeconds(+duration), DelimType.COLON)}
              </span>
            )}
            {cached && <Icon name='check-circle-outline' />}
            {<span>350 MB</span> && false /* TODO(sko) not ready yet */}
          </ItemDetailsContainer>
        }
      </ItemInfoContainer>
      {(hovered || showingMenu) && (
        <ContextualMenuAnchorButton
          items={[
            { name: 'Move', iconName: 'folder-exchange', onClick: () => {} },
            {
              name: 'Remove offline data',
              iconName: 'cloud-off',
              onClick: () => {}
            },
            {
              name: 'Remove from playlist',
              iconName: 'trash',
              onClick: () => {}
            }
          ]}
          onShowMenu={() => setShowingMenu(true)}
          onDismissMenu={() => setShowingMenu(false)}
        />
      )}
    </PlaylistItemContainer>
  )
}
