/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styled, { css } from 'styled-components'
import { useSelector } from 'react-redux'

import { color, font, spacing } from '@brave/leo/tokens/css/variables'
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
import { BouncingBars } from './bouncingBars'
import { useVerticallySortable } from '../utils/dragDropUtils'
import { ProgressBar } from './progressBar'

interface Props {
  playlist: Playlist
  item: PlaylistItemMojo
  isEditing: boolean
  isSelected?: boolean
  isHighlighted?: boolean
  canReorder: boolean
  forwardedRef?: React.ForwardedRef<HTMLAnchorElement>
  shouldBeHidden: boolean
  onClick: (item: PlaylistItemMojo) => void
}

const ThumbnailStyle = css`
  flex: 0 0 100px;
  height: 70px;
  object-fit: cover;
  border-radius: 4px;
  position: relative;
  overflow: hidden;
`
const StyledThumbnail = styled.div<{ src: string }>`
  ${ThumbnailStyle}
  background-image: url(${(p) => p.src});
  background-size: cover;
  background-position: center;
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

const PlaylistItemContainer = styled.li<{
  isActive: boolean
  isHighlighted?: boolean
  shouldBeHidden: boolean
}>`
  @keyframes highlightBackground {
    0% {
      background-color: none;
    }
    100% {
      background-color: ${color.container.interactive};
    }
  }

  display: flex;
  position: relative;
  padding: ${spacing.m} ${spacing.xl} ${spacing.m} ${spacing.m};
  height: 86px;
  align-items: center;
  gap: ${spacing.xl};
  user-select: none;

  ${(p) =>
    p.shouldBeHidden &&
    css`
      visibility: hidden;
    `}

  align-self: stretch;
  ${(p) =>
    p.isActive &&
    css`
      background: ${color.container.interactive};
    `}

  ${(p) =>
    p.isHighlighted &&
    css`
      animation: highlightBackground 500ms 4 alternate;
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
  font: ${font.default.semibold};
  cursor: default;
  max-height: 48px;
  overflow: hidden;
`

const ItemDetailsContainer = styled.div`
  display: flex;
  align-items: center;
  gap: 8px;
  align-self: stretch;
  color: ${color.text.tertiary};
  font: ${font.small.regular};
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
    ${color.icon.default} ${(p) => p.progress + '%'},
    ${color.primary[20]} ${(p) => p.progress + '%'},
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
  color: ${(p) => p.color};
`

const StyledCheckBox = styled(Icon)<{ checked: boolean }>`
  --leo-icon-size: 16px;
  color: ${(p) => (p.checked ? color.icon.interactive : color.icon.default)};
`

function Thumbnail ({
  thumbnailUrl,
  isSelected,
  isPlaying,
  duration,
  lastPlayedPosition
}: {
  thumbnailUrl?: string
  isSelected: boolean
  isPlaying: boolean
  duration: string
  lastPlayedPosition: number
}) {
  const overlay = (
    <>
      {isPlaying && (
        <BouncingBarsContainer>
          <BouncingBars />
        </BouncingBarsContainer>
      )}
      {isSelected && !!+duration && (
        <ProgressBar progress={lastPlayedPosition / (+duration / 1e6)} />
      )}
    </>
  )

  return thumbnailUrl ? (
    <StyledThumbnail src={thumbnailUrl}>{overlay}</StyledThumbnail>
  ) : (
    <DefaultThumbnail>{overlay}</DefaultThumbnail>
  )
}

export function PlaylistItem ({
  playlist,
  item,
  isEditing,
  isSelected,
  isHighlighted,
  shouldBeHidden,
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

  const [hovered, setHovered] = React.useState(false)
  const [showingMenu, setShowingMenu] = React.useState(false)
  const [focused, setFocused] = React.useState(false)

  const cachingProgress = useSelector<
    ApplicationState,
    CachingProgress | undefined
  >((applicationState) =>
    applicationState.playlistData?.cachingProgress?.get(id)
  )

  const currentItemId = useSelector<ApplicationState, string | undefined>(
    (applicationState) =>
      applicationState.playlistData?.lastPlayerState?.currentItem?.id
  )
  const playing = useSelector<ApplicationState, boolean | undefined>(
    (applicationState) =>
      applicationState.playlistData?.lastPlayerState?.playing
  )
  const isCurrentItem = currentItemId === item.id
  const isPlayingItem = isCurrentItem && playing

  return (
    <PlaylistItemContainer
      onFocus={() => setFocused(true)}
      onBlur={() => setFocused(false)}
      onMouseEnter={() => setHovered(true)}
      onMouseLeave={() => setHovered(false)}
      isActive={(isEditing && isSelected) || isCurrentItem}
      isHighlighted={isHighlighted}
      shouldBeHidden={shouldBeHidden}
      tabIndex={0}
      onKeyDown={(e) => e.key === 'Enter' && onClick(item)}
      onClick={() => onClick(item)}
    >
      {isEditing && (
        <StyledCheckBox
          checked={!!isSelected}
          name={isSelected ? 'check-circle-filled' : 'radio-unchecked'}
        />
      )}
      <Thumbnail
        thumbnailUrl={thumbnailUrl}
        isSelected={isCurrentItem}
        isPlaying={!!isPlayingItem}
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
      <ContextualMenuAnchorButton
        visible={(hovered || focused || showingMenu) && !isEditing}
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
            : cachingProgress
            ? undefined // TODO(sko) We may want to have "cancel caching".
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
          },
          {
            name: getLocalizedString(
              'bravePlaylistContextMenuViewOriginalPage'
            ),
            iconName: 'link-normal',
            onClick: () =>
              window.open(item.pageSource.url, '_blank', 'noopener noreferrer')
          }
        ]}
        onShowMenu={() => setShowingMenu(true)}
        onDismissMenu={() => setShowingMenu(false)}
      />
    </PlaylistItemContainer>
  )
}

export const SortablePlaylistItem = React.forwardRef(
  function SortablePlaylistItem (
    props: Props,
    forwardedRef?: React.ForwardedRef<HTMLAnchorElement>
  ) {
    const itemId = props.item.id
    const { attributes, listeners, setNodeRef, style } = useVerticallySortable({
      id: itemId,
      disabled: !props.canReorder
    })

    return (
      <div
        ref={setNodeRef}
        style={style}
        {...attributes}
        {...listeners}
        tabIndex={-1} // disables tab traversal for sortable as we have our own tab traversal logic
        aria-hidden='true'
      >
        <a
          ref={forwardedRef}
          href={`#${itemId}`}
          tabIndex={-1} // disables tab traversal for <a> as we only use this for auto scrolling.
          aria-hidden='true'
        />
        <PlaylistItem {...props} />
      </div>
    )
  }
)
