/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as PlaylistMojo from 'gen/brave/components/playlist/mojom/playlist.mojom.m.js'
import { Url } from 'gen/url/mojom/url.mojom.m.js'

type PlaylistEventListener = (event: PlaylistMojo.PlaylistEvent) => void

interface API {
  pageCallbackRouter: PlaylistMojo.PageCallbackRouter
  pageHandler: PlaylistMojo.PageHandlerRemote

  getAllPlaylists: () => Promise<{playlists: PlaylistMojo.Playlist[]}>
  getPlaylist: (id: string) => Promise<{playlist?: PlaylistMojo.Playlist}>
  addMediaFilesFromPageToPlaylist: (playlistId: string, url: string) => void
  removeItemFromPlaylist: (playlistId: string, itemId: string) => void

  addEventListener: (listener: PlaylistEventListener) => void
}

let playlistAPIInstance: API

class PlaylistAPIInstance implements API {
  pageCallbackRouter = new PlaylistMojo.PageCallbackRouter()
  pageHandler = new PlaylistMojo.PageHandlerRemote()

  constructor () {
    const factory = PlaylistMojo.PageHandlerFactory.getRemote()
    factory.createPageHandler(
        this.pageCallbackRouter.$.bindNewPipeAndPassRemote(),
        this.pageHandler.$.bindNewPipeAndPassReceiver())
  }

  getAllPlaylists = async function ():
      Promise<{playlists: PlaylistMojo.Playlist[]}> {
        return this.pageHandler.getAllPlaylists()
      }

  getPlaylist = async function (id: string):
  Promise<{playlist?: PlaylistMojo.Playlist}> {
    return this.pageHandler.getPlaylist(id)
  }

  addEventListener (listener: PlaylistEventListener) {
    this.pageCallbackRouter.onEvent.addListener(listener)
  }

  addMediaFilesFromPageToPlaylist (playlistId: string, url: string) {
    let mojoUrl = new Url()
    mojoUrl.url = url
    this.pageHandler.addMediaFilesFromPageToPlaylist(
        playlistId, mojoUrl)
  }

  removeItemFromPlaylist (playlistId: string, itemId: string) {
    this.pageHandler.removeItemFromPlaylist(playlistId, itemId)
  }
}

export function getPlaylistAPI (): API {
  if (!playlistAPIInstance) {
    playlistAPIInstance = new PlaylistAPIInstance()
  }
  return playlistAPIInstance
}
