/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  PageHandlerFactory,
  Playlist,
  PlaylistEvent,
  PlaylistServiceObserverCallbackRouter,
  PlaylistServiceRemote,
  PlaylistNativeUIRemote
} from 'gen/brave/components/playlist/common/mojom/playlist.mojom.m.js'

import { Url } from 'gen/url/mojom/url.mojom.m.js'
import { getPlaylistActions } from './getPlaylistActions'

type PlaylistEventListener = (event: PlaylistEvent) => void

let apiInstance: API

class API {
  #pageCallbackRouter = new PlaylistServiceObserverCallbackRouter()
  #pageHandler = new PlaylistServiceRemote()
  #nativeUI = new PlaylistNativeUIRemote()

  constructor () {
    const factory = PageHandlerFactory.getRemote()
    factory.createPageHandler(
      this.#pageCallbackRouter.$.bindNewPipeAndPassRemote(),
      this.#pageHandler.$.bindNewPipeAndPassReceiver(),
      this.#nativeUI.$.bindNewPipeAndPassReceiver()
    )
  }

  async getAllPlaylists () {
    return this.#pageHandler.getAllPlaylists()
  }

  async getPlaylist (id: string) {
    return this.#pageHandler.getPlaylist(id)
  }

  createPlaylist (playlist: Playlist) {
    this.#pageHandler.createPlaylist(playlist)
  }

  renamePlaylist (playlistId: string, newName: string) {
    this.#pageHandler
      .renamePlaylist(playlistId, newName)
      .then(({ updatedPlaylist }) => {
        getPlaylistActions().playlistUpdated(updatedPlaylist)
      })
  }

  removePlaylist (playlistId: string) {
    this.#pageHandler.removePlaylist(playlistId)
  }

  addMediaFilesFromPageToPlaylist (playlistId: string, url: string) {
    let mojoUrl = new Url()
    mojoUrl.url = url
    this.#pageHandler.addMediaFilesFromPageToPlaylist(
      playlistId,
      mojoUrl,
      /* canCache */ true
    )
  }

  addMediaFilesFromActiveTabToPlaylist (playlistId: string) {
    this.#pageHandler.addMediaFilesFromActiveTabToPlaylist(
      playlistId,
      /* canCache */ true
    )
  }

  moveItemFromPlaylist (playlistId: string, itemId: string[]) {
    this.#nativeUI.showMoveItemsUI(playlistId, itemId)
  }

  removeItemFromPlaylist (playlistId: string, itemId: string) {
    this.#pageHandler.removeItemFromPlaylist(playlistId, itemId)
  }

  recoverLocalData (playlistItemId: string) {
    this.#pageHandler.recoverLocalDataForItem(
      playlistItemId,
      /* updatePageUrlBeforeRecovery= */ false
    )
  }

  updateItemLastPlayedPosition (
    playlistItemId: string,
    lastPlayedPosition: number
  ) {
    this.#pageHandler.updateItemLastPlayedPosition(
      playlistItemId,
      lastPlayedPosition
    )
  }

  removeLocalData (playlistItemId: string) {
    this.#pageHandler.removeLocalDataForItem(playlistItemId)
  }

  showCreatePlaylistUI () {
    this.#nativeUI.showCreatePlaylistUI()
  }

  showRemovePlaylistUI (playlistId: string) {
    this.#nativeUI.showRemovePlaylistUI(playlistId)
  }

  reorderItemFromPlaylist (
    playlistId: string,
    itemId: string,
    position: number,
    callback: (result: boolean) => void
  ) {
    this.#pageHandler
      .reorderItemFromPlaylist(playlistId, itemId, position)
      .then(({ result }) => callback(result))
  }

  reorderPlaylist (
    playlistId: string,
    position: number,
    callback: (result: boolean) => void
  ) {
    this.#pageHandler
      .reorderPlaylist(playlistId, position)
      .then(({ result }) => callback(result))
  }

  // Events --------------------------------------------------------------------
  addEventListener (listener: PlaylistEventListener) {
    this.#pageCallbackRouter.onEvent.addListener(listener)
  }

  addMediaCachingProgressListener (
    listener: (
      id: string,
      totalBytes: bigint,
      receivedBytes: bigint,
      percentComplete: number,
      timeRemaining: string
    ) => void
  ) {
    this.#pageCallbackRouter.onMediaFileDownloadProgressed.addListener(listener)
  }

  addPlaylistUpdatedListener (listener: (playlist: Playlist) => void) {
    this.#pageCallbackRouter.onPlaylistUpdated.addListener(listener)
  }
}

export function getPlaylistAPI (): API {
  if (!apiInstance) {
    apiInstance = new API()
  }
  return apiInstance
}
