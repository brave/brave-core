/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as PlaylistMojo from 'gen/brave/components/playlist/mojom/playlist.mojom.m.js'
import { Url } from 'gen/url/mojom/url.mojom.m.js'

type PlaylistEventListener = (event: PlaylistMojo.PlaylistEvent) => void

let apiInstance: API

class API {
  #pageCallbackRouter = new PlaylistMojo.PageCallbackRouter()
  #pageHandler = new PlaylistMojo.PageHandlerRemote()

  constructor () {
    const factory = PlaylistMojo.PageHandlerFactory.getRemote()
    factory.createPageHandler(
        this.#pageCallbackRouter.$.bindNewPipeAndPassRemote(),
        this.#pageHandler.$.bindNewPipeAndPassReceiver())
  }

  async getAllPlaylists () {
    return this.#pageHandler.getAllPlaylists()
  }

  async getPlaylist (id: string) {
    return this.#pageHandler.getPlaylist(id)
  }

  createPlaylist (playlist: PlaylistMojo.Playlist) {
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
        playlistId, mojoUrl)
  }

  addMediaFilesFromOpenTabsToPlaylist (playlistId: string) {
    this.#pageHandler.addMediaFilesFromOpenTabsToPlaylist(
        playlistId)
  }

  removeItemFromPlaylist (playlistId: string, itemId: string) {
    this.#pageHandler.removeItemFromPlaylist(playlistId, itemId)
  }

  recoverLocalData (playlistItemId: string) {
    this.#pageHandler.recoverLocalDataForItem(playlistItemId)
  }

  removeLocalData (playlistItemId: string) {
    this.#pageHandler.removeLocalDataForItem(playlistItemId)
  }
}

export function getPlaylistAPI (): API {
  if (!apiInstance) {
    apiInstance = new API()
  }
  return apiInstance
}
