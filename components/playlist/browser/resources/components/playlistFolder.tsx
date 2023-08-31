// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { RouteComponentProps, Redirect } from 'react-router-dom'
import styled from 'styled-components'

import { color } from '@brave/leo/tokens/css'
import LeoButton from '@brave/leo/react/button'

import { PlaylistItem as PlaylistItemMojo } from 'gen/brave/components/playlist/common/mojom/playlist.mojom.m'

import PlaylistItem from './playlistItem'
import {
  PlaylistEditMode,
  useLastPlayerState,
  usePlaylist,
  usePlaylistEditMode
} from '../reducers/states'
import postMessageToPlayer from '../api/playerApi'
import { types } from '../constants/player_types'
import { getPlaylistActions } from '../api/getPlaylistActions'
import { getPlaylistAPI } from '../api/api'
import { ItemDragController } from './itemDragController'

interface MatchParams {
  playlistId: string
}

const StyledEditActionContainer = styled.div`
  display: flex;
  padding: 16px;
  align-items: center;
  gap: 16px;
  align-self: stretch;
  height: 68px;
  border-bottom: 1px solid ${color.divider.subtle};
`

const StyledEditButton = styled(LeoButton)`
  width: fit-content;
  flex-grow: 0;
  --leo-button-padding: 10px 12px;

  &:last-child {
    margin-left: auto;
  }
`

function exitEditMode () {
  getPlaylistActions().setPlaylistEditMode(undefined)
}

function EditActionsContainer ({
  playlistId,
  selectedIds
}: {
  playlistId: string
  selectedIds: Set<string>
}) {
  return (
    <StyledEditActionContainer>
      <StyledEditButton
        kind='plain-faint'
        size='small'
        isDisabled={!selectedIds.size}
        onClick={() => {
          if (selectedIds)
            getPlaylistAPI().moveItemFromPlaylist(playlistId, [...selectedIds])
          exitEditMode()
        }}
      >
        Move
      </StyledEditButton>
      <StyledEditButton
        kind='plain-faint'
        size='small'
        isDisabled={!selectedIds.size}
        onClick={() => {
          selectedIds.forEach(itemId =>
            getPlaylistAPI().removeItemFromPlaylist(playlistId, itemId)
          )
          exitEditMode()
        }}
      >
        Remove
      </StyledEditButton>
      <StyledEditButton
        kind='outline'
        size='small'
        onClick={() => exitEditMode()}
      >
        Done
      </StyledEditButton>
    </StyledEditActionContainer>
  )
}

export default function PlaylistFolder ({
  match
}: RouteComponentProps<MatchParams>) {
  const playlist = usePlaylist(match.params.playlistId)
  const lastPlayerState = useLastPlayerState()

  React.useEffect(() => {
    // When playlist updated and player is working, notify that the current
    // list has changed.
    if (playlist && playlist?.id === lastPlayerState?.currentList?.id) {
      postMessageToPlayer({
        actionType: types.SELECTED_PLAYLIST_UPDATED,
        currentList: playlist
      })
    }
  }, [playlist, lastPlayerState?.currentList?.id])

  const [selectedSet, setSelectedSet] = React.useState(new Set<string>())
  const editMode = usePlaylistEditMode()

  React.useEffect(() => {
    if (editMode !== PlaylistEditMode.BULK_EDIT)
      setSelectedSet(new Set<string>())
  }, [editMode])

  const dragControllerRef = React.useRef(new ItemDragController())

  // Share single callback among multiple items.
  const onItemClick = React.useCallback(
    item => {
      if (draggedOrder) setDraggedOrder(undefined)

      if (!playlist) return

      if (editMode === PlaylistEditMode.BULK_EDIT) {
        const newSelectedSet = new Set(selectedSet)
        if (newSelectedSet.has(item.id)) {
          newSelectedSet.delete(item.id)
        } else {
          newSelectedSet.add(item.id)
        }
        setSelectedSet(newSelectedSet)
        return
      }

      postMessageToPlayer({
        actionType: types.PLAYLIST_ITEM_SELECTED,
        currentList: playlist,
        currentItem: item
      })
    },
    [playlist, selectedSet, editMode]
  )

  const [draggedOrder, setDraggedOrder] = React.useState<
    PlaylistItemMojo[] | undefined
  >(undefined)

  dragControllerRef.current.onClickItem = onItemClick
  dragControllerRef.current.onDragItemStarted = () => {}
  dragControllerRef.current.onDragItemUpdated = (item, indexDelta) => {
    if (!playlist) return

    const index = playlist.items.findIndex(i => i.id === item.id)
    if (index + indexDelta === draggedOrder?.findIndex(i => i.id === item.id)) {
      return
    }

    let newOrder = [...playlist.items]
    newOrder.splice(index + indexDelta, 0, newOrder.splice(index, 1)[0])
    setDraggedOrder(newOrder)
  }
  dragControllerRef.current.onDragItemEnded = item => {
    if (!playlist) return

    const index = draggedOrder!.findIndex(i => i.id === item.id)
    getPlaylistAPI().reorderItemFromPlaylist(playlist.id!, item.id, index)
    setDraggedOrder(undefined)
  }
  dragControllerRef.current.canDrag =
    editMode !== PlaylistEditMode.BULK_EDIT && !lastPlayerState?.shuffleEnabled

  if (!playlist) {
    // After deleting a playlist from header, this could happen. In this case,
    // redirect to the index page.
    return <Redirect to='/' />
  }

  const itemsToRender =
    draggedOrder ??
    (lastPlayerState?.shuffleEnabled
      ? lastPlayerState.currentList?.items ?? playlist?.items
      : playlist?.items)

  return (
    <>
      {editMode === PlaylistEditMode.BULK_EDIT && (
        <EditActionsContainer
          playlistId={playlist.id!}
          selectedIds={selectedSet}
        />
      )}
      {itemsToRender.map(item => (
        <PlaylistItem
          key={item.id}
          playlist={playlist}
          item={item}
          isEditing={editMode === PlaylistEditMode.BULK_EDIT}
          isSelected={selectedSet.has(item.id)}
          dragController={dragControllerRef.current}
        />
      ))}
    </>
  )
}
