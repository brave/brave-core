// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { getLocale } from '$web-common/locale'

export type Message =
  | 'braveDefaultPlaylistName'
  | 'bravePlaylistContextMenuEdit'
  | 'bravePlaylistContextMenuShare'
  | 'bravePlaylistContextMenuKeepForOfflinePlaying'
  | 'bravePlaylistContextMenuRemovePlayedContents'
  | 'bravePlaylistContextMenuMove'
  | 'bravePlaylistContextMenuRemoveOfflineData'
  | 'bravePlaylistContextMenuRemoveFromPlaylist'
  | 'bravePlaylistContextMenuRenamePlaylist'
  | 'bravePlaylistContextMenuDeletePlaylist'
  | 'bravePlaylistContextMenuViewOriginalPage'
  | 'bravePlaylistEmptyFolderMessage'
  | 'bravePlaylistA11YCreatePlaylistFolder'
  | 'bravePlaylistA11YOpenPlaylistSettings'
  | 'bravePlaylistA11YClosePanel'
  | 'bravePlaylistA11YPlay'
  | 'bravePlaylistA11YPause'
  | 'bravePlaylistA11YNext'
  | 'bravePlaylistA11YPrevious'
  | 'bravePlaylistA11YToggleMuted'
  | 'bravePlaylistA11YShuffle'
  | 'bravePlaylistA11YRewind'
  | 'bravePlaylistA11YForward'
  | 'bravePlaylistA11YClose'
  | 'bravePlaylistA11YLoopOff'
  | 'bravePlaylistA11YLoopOne'
  | 'bravePlaylistA11YLoopAll'
  | 'bravePlaylistFailedToPlayTitle'
  | 'bravePlaylistFailedToPlayDescription'
  | 'bravePlaylistFailedToPlayRecover'
  | 'bravePlaylistAlertDismiss'
  | 'bravePlaylistAddMediaFromPage'

export function getLocalizedString(message: Message) {
  return getLocale(message)
}
