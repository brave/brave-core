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
  | 'bravePlaylistTooltipCreatePlaylistFolder'
  | 'bravePlaylistTooltipOpenPlaylistSettings'
  | 'bravePlaylistTooltipClosePanel'
  | 'bravePlaylistTooltipPlay'
  | 'bravePlaylistTooltipPause'
  | 'bravePlaylistTooltipNext'
  | 'bravePlaylistTooltipPrevious'
  | 'bravePlaylistTooltipToggleMuted'
  | 'bravePlaylistTooltipShuffle'
  | 'bravePlaylistTooltipRewind'
  | 'bravePlaylistTooltipForward'
  | 'bravePlaylistTooltipClose'
  | 'bravePlaylistTooltipLoopOff'
  | 'bravePlaylistTooltipLoopOne'
  | 'bravePlaylistTooltipLoopAll'
  | 'bravePlaylistFailedToPlayTitle'
  | 'bravePlaylistFailedToPlayDescription'
  | 'bravePlaylistFailedToPlayRecover'
  | 'bravePlaylistAlertDismiss'
  | 'bravePlaylistAddMediaFromPage'

export function getLocalizedString(message: Message) {
  return getLocale(message)
}
