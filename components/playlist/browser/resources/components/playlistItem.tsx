/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styled, { css } from 'styled-components'
import { useSelector } from 'react-redux'

import { color, font } from '@brave/leo/tokens/css'
import Icon from '@brave/leo/react/icon'

import DefaultThumbnailIcon from '../assets/playlist-thumbnail-icon.svg'
import ContextualMenuAnchorButton from './contextualMenu'
import {
  Playlist,
  PlaylistItem as PlaylistItemMojo
} from 'gen/brave/components/playlist/common/mojom/playlist.mojom.m.js'
import {
  formatTimeInSeconds,
  microSecondsToSeconds
} from '../utils/timeFormatter'
import { getLocalizedString } from '../utils/l10n'
import { formatBytes } from '../utils/bytesFormatter'
import { ApplicationState, CachingProgress } from '../reducers/states'
import { getPlaylistAPI } from '../api/api'

interface Props {
  playlist: Playlist
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
  font: ${font.primary.default.semibold};
  cursor: default;
`

const ItemDetailsContainer = styled.div`
  display: flex;
  align-items: center;
  gap: 8px;
  align-self: stretch;
  color: ${color.text.tertiary};
  font: ${font.primary.small.regular};
`

const CachedIcon = styled(Icon)`
  --leo-icon-size: 16px;
`

const ProgressCircle = styled.div<{ progress: number }>`
  --leo-icon-size: 12px;

  width: 16px;
  height: 16px;
  position: relative;
  background: conic-gradient(
    ${color.icon.default} 0%,
    ${color.icon.default} ${p => p.progress + '%'},
    ${color.primary[20]} ${p => p.progress + '%'},
    ${color.primary[20]} 100%
  );
  clip-path: circle();

  &:before {
    background-color: ${color.container.background};
    position: absolute;
    content: '';
    width: 100%;
    height: 100%;
    clip-path: circle(6px);
  }

  & > leo-icon {
    position: absolute;
    width: var(--leo-icon-size);
    height: var(--leo-icon-size);
    top: calc((100% - var(--leo-icon-size)) / 2);
    left: calc((100% - var(--leo-icon-size)) / 2);
  }
`

const ColoredSpan = styled.span<{ color: any }>`
  color: ${p => p.color};
`

export default function PlaylistItem ({
  playlist,
  item: {
    id,
    name,
    cached,
    mediaFileBytes,
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

  const cachingProgress = useSelector<
    ApplicationState,
    CachingProgress | undefined
  >(applicationState => applicationState.playlistData?.cachingProgress?.get(id))

  return (
    <PlaylistItemContainer
      onClick={() => !showingMenu && onClick(id)}
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
                {formatTimeInSeconds(microSecondsToSeconds(+duration), 'colon')}
              </span>
            )}
            {cached ? (
              <>
                <CachedIcon name='check-circle-outline' />
                {mediaFileBytes && <span>{formatBytes(mediaFileBytes)}</span>}
              </>
            ) : (
              cachingProgress && (
                <>
                  <ProgressCircle progress={cachingProgress.percentComplete}>
                    <Icon name='arrow-down' />
                  </ProgressCircle>
                  <ColoredSpan color={color.text.interactive}>
                    {formatBytes(cachingProgress.receivedBytes)}
                  </ColoredSpan>
                </>
              )
            )}
          </ItemDetailsContainer>
        }
      </ItemInfoContainer>
      {(hovered || showingMenu) && (
        <ContextualMenuAnchorButton
          items={[
            {
              name: getLocalizedString('bravePlaylistContextMenuMove'),
              iconName: 'folder-exchange',
              onClick: () =>
                getPlaylistAPI().moveItemFromPlaylist(playlist.id!, [id])
            },
            cached
              ? {
                  name: getLocalizedString(
                    'bravePlaylistContextMenuRemoveOfflineData'
                  ),
                  iconName: 'cloud-off',
                  onClick: () => getPlaylistAPI().removeLocalData(id)
                }
              : {
                  name: getLocalizedString(
                    'bravePlaylistContextMenuKeepForOfflinePlaying'
                  ),
                  iconName: 'cloud-download',
                  onClick: () => getPlaylistAPI().recoverLocalData(id)
                },
            {
              name: getLocalizedString(
                'bravePlaylistContextMenuRemoveFromPlaylist'
              ),
              iconName: 'trash',
              onClick: () =>
                getPlaylistAPI().removeItemFromPlaylist(playlist.id!, id)
            }
          ]}
          onShowMenu={() => setShowingMenu(true)}
          onDismissMenu={() => setShowingMenu(false)}
        />
      )}
    </PlaylistItemContainer>
  )
}
