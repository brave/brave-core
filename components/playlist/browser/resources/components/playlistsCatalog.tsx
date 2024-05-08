// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled, { css } from 'styled-components'
import { Link } from 'react-router-dom'

import {
  DndContext,
  DragEndEvent,
  DragOverlay,
  DragStartEvent
} from '@dnd-kit/core'
import {
  SortableContext,
  arrayMove,
  verticalListSortingStrategy
} from '@dnd-kit/sortable'

import Icon from '@brave/leo/react/icon'
import { color, spacing } from '@brave/leo/tokens/css/variables'

import { Playlist } from 'gen/brave/components/playlist/common/mojom/playlist.mojom.m.js'

import PlayLaterCardOverlayImage from '../assets/playlater-card-overlay.svg'
import PlaylistInfo from './playlistInfo'
import { useSelector } from 'react-redux'
import {
  ApplicationState,
  useTotalDuration,
  useTotalSize
} from '../reducers/states'

import { getPlaylistAPI } from '../api/api'
import { getPlaylistActions } from '../api/getPlaylistActions'
import {
  restrictToVerticalAxis,
  useDraggedId,
  useDraggedOrder,
  useSensorsWithThreshold,
  useVerticallySortable
} from '../utils/dragDropUtils'

type ThumbnailProps = {
  isDefaultPlaylist: boolean
  thumbnailUrl: string | undefined
}

type PlaylistCardProps = {
  playlist: Playlist
  shouldBeHidden?: boolean
}

const StyledLink = styled(Link)`
  text-decoration: none;
  color: unset;
`

const PlaylistCardContainer = styled.div<
  ThumbnailProps & Omit<PlaylistCardProps, 'playlist'>
>`
  ${p =>
    p.shouldBeHidden &&
    css`
      visibility: hidden;
    `}

  /* Default Playlist background */
  ${p =>
    p.isDefaultPlaylist &&
    css`
      background: linear-gradient(150.64deg, #322fb4 0%, #3835ca 100%);
    `}

  /* Default background for playlist without thumbnail */
  ${p =>
    !p.isDefaultPlaylist &&
    !p.thumbnailUrl &&
    css`
      background-color: ${color.container.highlight};
    `}

  /* Playlist with Thumbnail*/
  ${p =>
    !p.isDefaultPlaylist &&
    p.thumbnailUrl &&
    css`
      overflow: clip;
      &::before {
        position: absolute;
        content: '';
        background: linear-gradient(
            0deg,
            rgba(0, 0, 0, 0.4),
            rgba(0, 0, 0, 0.4)
          ),
          url(${p.thumbnailUrl});
        background-size: cover;
        filter: blur(8px);
        width: 100%;
        height: 100%;
        z-index: -1;
        margin: calc(-1 * ${spacing.m});
      }
    `}
  border-radius: 8px;
  padding: ${spacing.m};

  position: relative;
  display: flex;
  gap: 16px;
`

const PlayLaterCardOverlay = styled.div`
  position: absolute;
  top: 0;
  right: 0;
  content: url(${PlayLaterCardOverlayImage});
`

const ColoredPlaylistInfo = styled(PlaylistInfo)<{ hasBackground: boolean }>`
  color: ${({ hasBackground }) =>
    hasBackground ? color.primitive.gray[1] : color.text.primary};
`

const PlaylistThumbnailContainer = styled.div<ThumbnailProps>`
  /* Default Playlist icon */
  ${p =>
    p.isDefaultPlaylist &&
    css`
      background-color: ${color.button.background};
      color: white;
    `}

  /* Default icon for playlist without thumbnail */
  ${p =>
    !p.isDefaultPlaylist &&
    !p.thumbnailUrl &&
    css`
      --leo-icon-color: ${color.icon.default};
      background-color: ${color.container.background};
    `}

  /* Playlist with Thumbnail*/
  ${p =>
    !p.isDefaultPlaylist &&
    !!p.thumbnailUrl &&
    css`
      background-image: url(${p.thumbnailUrl});
      background-size: cover;
      background-repeat: no-repeat;
      background-position: center;
    `}

  width: 80px;
  height: 80px;
  flex: 0 0 auto;

  display: flex;
  justify-content: center;
  align-items: center;
`
function PlaylistThumbnail (props: ThumbnailProps) {
  return (
    <PlaylistThumbnailContainer {...props}>
      {(props.isDefaultPlaylist || !props.thumbnailUrl) && (
        <Icon name={props.isDefaultPlaylist ? 'history' : 'product-playlist'} />
      )}
    </PlaylistThumbnailContainer>
  )
}

// A card UI for representing a playlist.
function PlaylistCard ({ playlist, shouldBeHidden }: PlaylistCardProps) {
  const isDefaultPlaylist = playlist.id === 'default'
  const thumbnailUrl = React.useMemo(() => {
    return playlist.items?.find(item => item.thumbnailPath?.url)?.thumbnailPath
      .url
  }, [playlist])
  const hasBackground = isDefaultPlaylist || !!thumbnailUrl

  const totalDuration = useTotalDuration(playlist)
  const totalSize = useTotalSize(playlist)

  return (
    <StyledLink to={`/playlist/${playlist.id}`}>
      <PlaylistCardContainer
        isDefaultPlaylist={isDefaultPlaylist}
        thumbnailUrl={thumbnailUrl}
        shouldBeHidden={shouldBeHidden}
      >
        {isDefaultPlaylist && <PlayLaterCardOverlay />}
        <PlaylistThumbnail
          isDefaultPlaylist={isDefaultPlaylist}
          thumbnailUrl={thumbnailUrl}
        />
        <ColoredPlaylistInfo
          playlistName={playlist.name}
          isDefaultPlaylist={isDefaultPlaylist}
          itemCount={playlist.items.length}
          totalDuration={totalDuration}
          totalSize={totalSize}
          hasBackground={hasBackground}
        />
      </PlaylistCardContainer>
    </StyledLink>
  )
}

function SortablePlaylistCard (props: PlaylistCardProps) {
  const { attributes, listeners, setNodeRef, style } = useVerticallySortable({
    id: props.playlist.id!
  })

  return (
    <div
      ref={setNodeRef}
      style={style}
      {...attributes}
      {...listeners}
      tabIndex={-1} // Disables tab traversal for sortable as we have StyledLink in PlaylistCard.
      aria-hidden='true'
    >
      <PlaylistCard {...props} />
    </div>
  )
}

const PlaylistsCatalogFlexBox = styled.div`
  display: flex;
  flex-direction: column;
  gap: 8px;
  padding: 8px;
`

// A catalog that shows all playlists
export default function PlaylistsCatalog () {
  const playlists = useSelector<ApplicationState, Playlist[] | undefined>(
    applicationState => applicationState.playlistData?.lists
  )

  const [draggedId, setDraggedId] = useDraggedId()
  const [draggedOrder, setDraggedOrder] = useDraggedOrder<Playlist>()

  const sensors = useSensorsWithThreshold()

  return (
    <DndContext
      sensors={sensors}
      onDragStart={(event: DragStartEvent) => {
        setDraggedId(event.active.id)
        setDraggedOrder(playlists ? [...playlists] : [])
      }}
      onDragEnd={(event: DragEndEvent) => {
        setDraggedId(null)
        if (!draggedOrder) return

        const { active, over } = event
        if (!over || active.id === over.id) {
          setDraggedOrder(null)
          return
        }

        if (active.id !== over.id) {
          const oldIndex = draggedOrder.findIndex(i => i.id === active.id)
          const newIndex = draggedOrder.findIndex(i => i.id === over.id)
          // Lock the order until updating completes.
          setDraggedOrder(arrayMove(draggedOrder, oldIndex, newIndex))

          getPlaylistAPI().reorderPlaylist('' + draggedId, newIndex, result => {
            if (!result) {
              setDraggedOrder(null)
              return
            }

            // Reload playlists with new order
            getPlaylistAPI()
              .getAllPlaylists()
              .then(({ playlists }) => {
                getPlaylistActions().playlistLoaded(playlists)
                setDraggedOrder(null)
              })
          })
        }
      }}
      onDragCanceled={() => {
        setDraggedId(null)
      }}
    >
      <PlaylistsCatalogFlexBox>
        <SortableContext
          items={(draggedOrder ?? playlists)?.map(p => p.id!) ?? []}
          strategy={verticalListSortingStrategy}
        >
          {(draggedOrder ?? playlists)?.map((playlist: Playlist) => {
            return (
              <SortablePlaylistCard
                key={playlist.id}
                playlist={playlist}
                shouldBeHidden={draggedId === playlist.id}
              />
            )
          })}
        </SortableContext>
      </PlaylistsCatalogFlexBox>
      <DragOverlay modifiers={restrictToVerticalAxis}>
        {playlists && draggedId && (
          <PlaylistCard playlist={playlists.find(p => p.id === draggedId)!} />
        )}
      </DragOverlay>
    </DndContext>
  )
}
