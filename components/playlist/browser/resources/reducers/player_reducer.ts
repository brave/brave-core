// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { Reducer } from 'redux'

import { PlayerState } from '../reducers/states'
import { types } from '../constants/player_types'
import { SelectedPlaylistUpdatedPayload } from '../api/playerApi'

const playerReducer: Reducer<PlayerState | undefined> = (
  state: PlayerState | undefined,
  action
) => {
  if (state === undefined) {
    state = { currentList: undefined, currentItem: undefined, playing: false }
  }

  switch (action.type) {
    case types.PLAYLIST_ITEM_SELECTED: {
      const { currentItem, currentList } = action.payload
      console.assert(!currentItem || currentList?.items.includes(currentItem))
      state = { ...state, currentItem, currentList }
      break
    }

    case types.SELECTED_PLAYLIST_UPDATED: {
      // TODO(sko) When we implement the "Shuffle" feature, we might need to
      // migration code to the shuffled order stable.
      const { currentList } = action.payload as SelectedPlaylistUpdatedPayload
      const currentItem = currentList.items.find(
        e => e.id === state?.currentItem?.id
      )
      state = { ...state, currentList, currentItem }
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
  }
  return state
}

export default playerReducer
