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

  removePlaylist (playlistId: string) {
    this.#pageHandler.removePlaylist(playlistId)
  }

  addEventListener (listener: PlaylistEventListener) {
    this.#pageCallbackRouter.onEvent.addListener(listener)
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

  removeItemFromPlaylist (playlistId: string, itemId: string) {
    this.#pageHandler.removeItemFromPlaylist(playlistId, itemId)
  }

  recoverLocalData (playlistItemId: string) {
    this.#pageHandler.recoverLocalDataForItem(
      playlistItemId,
      /* updatePageUrlBeforeRecovery= */ false
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
}

export function getPlaylistAPI (): API {
  if (!apiInstance) {
    apiInstance = new API()
  }
  return apiInstance
}
