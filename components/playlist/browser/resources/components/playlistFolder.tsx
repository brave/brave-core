// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { RouteComponentProps, Redirect } from 'react-router-dom'
import styled from 'styled-components'
import {
  DndContext,
  DragStartEvent,
  DragOverlay,
  UniqueIdentifier,
  DragEndEvent,
  useSensor,
  TouchSensor,
  PointerSensor,
  MouseSensor,
  useSensors
} from '@dnd-kit/core'
import {
  arrayMove,
  SortableContext,
  verticalListSortingStrategy
} from '@dnd-kit/sortable'
import { Transform } from '@dnd-kit/utilities'

import { color } from '@brave/leo/tokens/css'
import LeoButton from '@brave/leo/react/button'

import { PlaylistItem as PlaylistItemMojo } from 'gen/brave/components/playlist/common/mojom/playlist.mojom.m'

import { SortablePlaylistItem, PlaylistItem } from './playlistItem'
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

  const [draggedId, setDraggedId] = React.useState<UniqueIdentifier | null>(
    null
  )

  React.useEffect(() => {
    if (editMode !== PlaylistEditMode.BULK_EDIT)
      setSelectedSet(new Set<string>())
  }, [editMode])

  // Share single callback among multiple items.
  const onItemClick = React.useCallback(
    item => {
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
    PlaylistItemMojo[] | null
  >(null)

  const sensors = useSensors(
    useSensor(MouseSensor, {
      activationConstraint: { distance: 40 }
    }),
    useSensor(PointerSensor, {
      activationConstraint: { distance: 40 }
    }),
    useSensor(TouchSensor, {
      activationConstraint: {
        delay: 250,
        tolerance: 40
      }
    })
  )

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

  const getPlaylistItem = (
    item: PlaylistItemMojo | undefined,
    isDraggable: boolean,
    shouldBeHidden: boolean
  ) => {
    if (!item) return null

    if (!isDraggable) {
      return (
        <PlaylistItem
          key={item.id}
          playlist={playlist}
          item={item}
          isEditing={editMode === PlaylistEditMode.BULK_EDIT}
          isSelected={selectedSet.has(item.id)}
          shouldBeHidden={shouldBeHidden}
          onClick={onItemClick}
        />
      )
    }

    return (
      <SortablePlaylistItem
        key={item.id}
        playlist={playlist}
        item={item}
        isEditing={editMode === PlaylistEditMode.BULK_EDIT}
        isSelected={selectedSet.has(item.id)}
        shouldBeHidden={shouldBeHidden}
        onClick={onItemClick}
      />
    )
  }

  const WrapWithDragContext = (children: any) => {
    const canDrag =
      editMode !== PlaylistEditMode.BULK_EDIT &&
      !lastPlayerState?.shuffleEnabled
    return canDrag ? (
      <DndContext
        sensors={sensors}
        onDragStart={(event: DragStartEvent) => {
          setDraggedId(event.active.id)
          setDraggedOrder([...itemsToRender])
        }}
        onDragEnd={(event: DragEndEvent) => {
          setDraggedId(null)
          if (!draggedOrder) return

          const { active, over } = event
          if (!over) return

          if (active.id !== over.id) {
            const oldIndex = draggedOrder.findIndex(i => i.id === active.id)
            const newIndex = draggedOrder.findIndex(i => i.id === over.id)
            // Lock the order until updating completes.
            setDraggedOrder(arrayMove(draggedOrder, oldIndex, newIndex))

            getPlaylistAPI().reorderItemFromPlaylist(
              playlist.id!,
              '' + active.id,
              newIndex,
              () => setDraggedOrder(null)
            )
          }
        }}
        onDragCanceled={() => {
          setDraggedId(null)
        }}
      >
        <SortableContext
          items={itemsToRender}
          strategy={verticalListSortingStrategy}
        >
          {children}
        </SortableContext>
        <DragOverlay
          modifiers={[
            ({ transform }: { transform: Transform }) => {
              return { ...transform, x: 0 }
            }
          ]}
        >
          {getPlaylistItem(
            itemsToRender.find(i => i.id === draggedId),
            /* isDraggable */ true,
            /* shouldBeHidden */ false
          )}
        </DragOverlay>
      </DndContext>
    ) : (
      <>{children}</>
    )
  }

  return WrapWithDragContext(
    <>
      {editMode === PlaylistEditMode.BULK_EDIT && (
        <EditActionsContainer
          playlistId={playlist.id!}
          selectedIds={selectedSet}
        />
      )}
      {itemsToRender.map(i =>
        getPlaylistItem(
          i /* shouldBeHidden */,
          /* isDraggable */ editMode !== PlaylistEditMode.BULK_EDIT &&
            !lastPlayerState?.shuffleEnabled,
          /* shouldBeHidden */ i.id === draggedId
        )
      )}
    </>
  )
}
