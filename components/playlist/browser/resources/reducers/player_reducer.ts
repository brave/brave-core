// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { Reducer } from 'redux'

import { PlaylistItem } from 'gen/brave/components/playlist/common/mojom/playlist.mojom.m'

import { PlayerState } from '../reducers/states'
import { types } from '../constants/player_types'
import { SelectedPlaylistUpdatedPayload } from '../api/playerApi'

function shuffleItems (
  itemsInOrder: PlaylistItem[],
  currentItem: PlaylistItem | undefined
) {
  const shuffledItems = currentItem
    ? itemsInOrder.splice(itemsInOrder.indexOf(currentItem), 1)
    : []

  while (itemsInOrder.length) {
    const randomIndex = Math.floor(Math.random() * itemsInOrder.length)
    const randomItem = itemsInOrder.splice(randomIndex, 1)[0]
    shuffledItems.push(randomItem)
  }

  return shuffledItems
}

const playerReducer: Reducer<PlayerState | undefined> = (
  state: PlayerState | undefined,
  action
) => {
  if (state === undefined) {
    state = {
      currentList: undefined,
      itemsInOrder: undefined,
      currentItem: undefined,
      playing: false,
      shuffleEnabled: false,
      autoPlayEnabled: true
    }
  }

  switch (action.type) {
    case types.PLAYLIST_ITEM_SELECTED: {
      const { currentItem, currentList } = action.payload
      console.assert(!currentItem || currentList?.items.includes(currentItem))
      const shouldShuffle =
        state.currentList?.id !== currentList.id && state.shuffleEnabled
      if (shouldShuffle) {
        state = {
          ...state,
          currentItem,
          currentList: {
            ...currentList,
            items: shuffleItems([...currentList.items], currentItem)
          },
          itemsInOrder: [...currentList.items]
        }
      } else {
        state = { ...state, currentItem, currentList }
      }

      break
    }

    case types.SELECTED_PLAYLIST_UPDATED: {
      const { currentList } = action.payload as SelectedPlaylistUpdatedPayload
      const itemsInOrder = state.itemsInOrder ? currentList?.items : undefined

      if (
        state.shuffleEnabled &&
        currentList.items.length &&
        state.currentList?.items.length
      ) {
        // Record items for efficiency
        const currentItemsMap = currentList.items.reduce((map, item) => {
          map.set(item.id, item)
          return map
        }, new Map<string, PlaylistItem>())

        const oldItemIds = state.currentList.items.map(i => i.id)
        const oldItemsSet = new Set(oldItemIds)
        const removedItems = new Set(
          oldItemIds.filter(id => !currentItemsMap.has(id))
        )

        const newList = state.currentList.items
          // Reorder items from top frame in the shuffled order
          .filter(oldItem => !removedItems.has(oldItem.id))
          .map(oldItem => currentItemsMap.get(oldItem.id)!)
          // Append new items to the end
          .concat(currentList.items.filter(item => !oldItemsSet.has(item.id)))

        currentList.items = newList
      }

      const currentItem = currentList.items.find(
        e => e.id === state?.currentItem?.id
      )
      state = { ...state, currentList, currentItem, itemsInOrder }
      break
    }

    case types.PLAYER_STARTED_PLAYING_ITEM:
      state = { ...state, playing: true }
      break

    case types.PLAYER_STOPPED_PLAYING_ITEM:
      state = { ...state, playing: false }
      break

    case types.PLAYER_PLAY_NEXT_ITEM:
      if (state.currentList && state.currentItem) {
        const items = state.currentList.items
        const currentIndex = items.indexOf(state.currentItem)
        if (currentIndex === -1) {
          throw new Error("Couldn't find the index of the current item ")
        }

        if (currentIndex !== items.length - 1) {
          const currentItem = items[currentIndex + 1]
          state = { ...state, currentItem }
        }
      }
      break

    case types.PLAYER_PLAY_PREVIOUS_ITEM:
      if (state.currentList && state.currentItem) {
        const items = state.currentList.items
        const currentIndex = items.indexOf(state.currentItem)
        if (currentIndex === -1) {
          throw new Error("Couldn't find the index of the current item ")
        }

        if (currentIndex !== 0) {
          const currentItem = items[currentIndex - 1]
          state = { ...state, currentItem }
        }
      }
      break

    case types.PLAYER_TOGGLE_AUTO_PLAY: {
      state = { ...state, autoPlayEnabled: !state.autoPlayEnabled }
      break
    }

    case types.PLAYER_TOGGLE_SHUFFLE: {
      const shuffleEnabled = !state.shuffleEnabled
      if (state.currentList) {
        if (shuffleEnabled) {
          const shuffledItems = shuffleItems(
            [...state.currentList.items],
            state.currentItem
          )

          state.itemsInOrder = [...state.currentList.items]
          state.currentList = { ...state.currentList, items: shuffledItems }
        } else {
          state.currentList = {
            ...state.currentList,
            items: state.itemsInOrder!
          }
          state.itemsInOrder = undefined
        }
      }

      state = { ...state, shuffleEnabled }
      break
    }
  }
  return state
}

export default playerReducer
