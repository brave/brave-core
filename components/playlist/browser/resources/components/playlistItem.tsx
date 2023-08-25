/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styled, { css } from 'styled-components'
import { useSelector } from 'react-redux'

import { color, font } from '@brave/leo/tokens/css'
import Icon from '@brave/leo/react/icon'
import ProgressBar from '@brave/leo/react/progressBar'

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
import { BouncingBars } from './bouncingBars'

interface Props {
  playlist: Playlist
  item: PlaylistItemMojo
  isEditing: boolean
  isSelected?: boolean
  onClick: (item: PlaylistItemMojo) => void
}

const ThumbnailStyle = css`
  flex: 0 0 140px;
  height: 80px;
  object-fit: cover;
  border-radius: 4px;
  position: relative;
  overflow: hidden;
`
const StyledThumbnail = styled.div<{ src: string }>`
  ${ThumbnailStyle}
  background-image: url(${p => p.src});
  background-size: cover;
`

const StyledProgressBar = styled(ProgressBar)`
  position: absolute;
  bottom: 0;
  width: 100%;
  --leo-progressbar-radius: 0;
`

const BouncingBarsContainer = styled.div`
  position: absolute;
  left: 4px;
  bottom: 8px;
  filter: drop-shadow(0px 1px 4px rgba(0, 0, 0, 0.4));
`

const DefaultThumbnail = styled.div`
  ${ThumbnailStyle}
  content: url(${DefaultThumbnailIcon});
`

const PlaylistItemContainer = styled.li<{ isActive: boolean }>`
  display: flex;
  padding: 8px 16px 8px 8px;
  align-items: center;
  gap: 16px;
  align-self: stretch;
  ${p =>
    p.isActive &&
    css`
      background: ${color.container.interactive};
    `}
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

const StyledCheckBox = styled(Icon)<{ checked: boolean }>`
  --leo-icon-size: 20px;
  color: ${p => (p.checked ? color.icon.interactive : color.icon.default)};
`

function Thumbnail ({
  thumbnailUrl,
  isPlaying,
  duration,
  lastPlayedPosition
}: {
  thumbnailUrl?: string
  isPlaying: boolean
  duration: string
  lastPlayedPosition: number
}) {
  const overlay = isPlaying ? (
    <>
      <BouncingBarsContainer>
        <BouncingBars />
      </BouncingBarsContainer>
      {!!+duration && (
        <StyledProgressBar
          progress={lastPlayedPosition / (+duration / 1e6)}
        ></StyledProgressBar>
      )}
    </>
  ) : null

  return thumbnailUrl ? (
    <StyledThumbnail src={thumbnailUrl}>{overlay}</StyledThumbnail>
  ) : (
    <DefaultThumbnail>{overlay}</DefaultThumbnail>
  )
}

export default function PlaylistItem ({
  playlist,
  item,
  isEditing,
  isSelected,
  onClick
}: Props) {
  const {
    id,
    name,
    cached,
    mediaFileBytes,
    duration,
    thumbnailPath: { url: thumbnailUrl }
  } = item

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

  const currentItemId = useSelector<ApplicationState, string | undefined>(
    applicationState =>
      applicationState.playlistData?.lastPlayerState?.currentItem?.id
  )
  const isPlaying = currentItemId === item.id

  return (
    <PlaylistItemContainer
      onClick={() => !showingMenu && onClick(item)}
      onMouseEnter={() => setHovered(true)}
      onMouseLeave={e => setHovered(false)}
      isActive={(isEditing && isSelected) || isPlaying}
    >
      <a ref={anchorElem} href={`#${id}`} />
      {isEditing && (
        <StyledCheckBox
          checked={!!isSelected}
          name={isSelected ? 'check-circle-filled' : 'radio-unchecked'}
        />
      )}
      <Thumbnail
        thumbnailUrl={thumbnailUrl}
        isPlaying={isPlaying}
        duration={item.duration}
        lastPlayedPosition={item.lastPlayedPosition}
      />
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
      {(hovered || showingMenu) && !isEditing && (
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
