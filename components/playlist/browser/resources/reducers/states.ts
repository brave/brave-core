/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { useSelector } from 'react-redux'

import {
  Playlist,
  PlaylistItem
} from 'gen/brave/components/playlist/common/mojom/playlist.mojom.m.js'

import { getItemDurationInSeconds } from '../utils/timeFormatter'
import { getFormattedTotalBytes } from '../utils/bytesFormatter'

export enum PlaylistEditMode {
  RENAME = 'rename',
  BULK_EDIT = 'bulkedit'
}

// For security reason, we're embedding player to an <iframe>. And these two
// states are mutually exclusive.
export interface ApplicationState {
  // Used by app.tsx
  playlistData: PlaylistData | undefined

  // Used by a player.tsx
  playerState: PlayerState | undefined
}

export interface CachingProgress {
  id: string
  totalBytes: bigint
  receivedBytes: bigint
  percentComplete: number
  timeRemaining: string
}

export interface PlaylistData {
  lists: Playlist[]
  currentList: Playlist | undefined
  // TODO(sko) Investigate if it's possible to remove this and use ApplicationState.playerState.
  lastPlayerState: PlayerState | undefined

  playlistEditMode: PlaylistEditMode | undefined

  cachingProgress: Map<string, CachingProgress>

  shouldShowAddMediaFromPage: boolean
}

type LoopMode = 'single-item' | 'all-items' | undefined

export interface PlayerState {
  // This list could be in different order from the original list when users
  // shuffle the list.
  currentList: Playlist | undefined
  // This keeps the original items order when |shuffleEnabled| is true
  itemsInOrder: PlaylistItem[] | undefined

  currentItem: PlaylistItem | undefined

  playing: boolean

  autoPlayEnabled: boolean

  shuffleEnabled: boolean

  loopMode: LoopMode
}

export const usePlaylist = (id?: string) =>
  useSelector<ApplicationState, Playlist | undefined>(applicationState =>
    applicationState.playlistData?.lists.find(e => e.id === id)
  )

export const useTotalDuration = (playlist?: Playlist) => {
  return React.useMemo(() => {
    // TODO(sko) Duration value could be missing. Only Youtube could work.
    //  * We need to update duration when Playlist player plays a video
    return playlist?.items?.reduce((sum, item) => {
      return sum + getItemDurationInSeconds(item)
    }, 0)
  }, [playlist])
}

export function useTotalSize (playlist?: Playlist) {
  return React.useMemo(() => {
    return getFormattedTotalBytes(playlist?.items ?? [])
  }, [playlist])
}

export function usePlaylistEditMode () {
  return useSelector<ApplicationState, PlaylistEditMode | undefined>(
    applicationState => applicationState.playlistData?.playlistEditMode
  )
}

export function useLastPlayerState () {
  return useSelector<ApplicationState, PlayerState | undefined>(
    applicationState => applicationState.playlistData?.lastPlayerState
  )
}

export function useAutoPlayEnabled () {
  return useSelector<ApplicationState, boolean | undefined>(
    applicationState => applicationState.playerState?.autoPlayEnabled
  )
}

export function useInitialized() {
  return useSelector<ApplicationState, boolean>(
    applicationState => !!applicationState.playlistData?.lists.length
  )
}

export function useLoopMode () {
  return useSelector<ApplicationState, LoopMode>(
    applicationState => applicationState.playerState?.loopMode
  )
}

export function useShouldShowAddMediaFromPage () {
  return useSelector<ApplicationState, boolean | undefined>(
    applicationState => applicationState.playlistData?.shouldShowAddMediaFromPage
  )
}
