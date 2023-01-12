/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as PlaylistMojo from 'gen/brave/components/playlist/common/mojom/playlist.mojom.m.js'

declare namespace Playlist {
  export interface ApplicationState {
    playlistData: State|undefined
  }

  export interface State {
    lists : PlaylistMojo.Playlist[]
    currentList: PlaylistMojo.Playlist|undefined
  }
}
