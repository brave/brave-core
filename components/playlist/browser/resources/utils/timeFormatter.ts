// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { PlaylistItem } from 'gen/brave/components/playlist/common/mojom/playlist.mojom.m.js'

export type DelimType = 'colon' | 'space'

export function getItemDurationInSeconds(item: PlaylistItem) {
  // item.duration is in microseconds
  if (!item.duration) {
    return 0
  }

  return microSecondsToSeconds(+item.duration)
}

export function microSecondsToSeconds(timeInMicroseconds: number) {
  return Math.floor(timeInMicroseconds / 1_000_000)
}

export function formatTimeInSeconds(timeInSeconds: number, delim: DelimType) {
  if (Number.isNaN(timeInSeconds)) timeInSeconds = 0

  const hours = Math.floor(timeInSeconds / 3600)
  const minutes = Math.floor((timeInSeconds % 3600) / 60)
  const seconds = Math.floor(timeInSeconds % 60)
  if (delim === 'colon') {
    const stringFormat = (t: number) => String(t).padStart(2, '0')
    const parts = [stringFormat(minutes), stringFormat(seconds)]
    if (hours) {
      parts.unshift(stringFormat(hours))
    }
    return parts.join(':')
  }

  const parts = [`${minutes ?? 0}m`, `${seconds ?? 0}s`]
  if (hours) parts.unshift(`${hours}h`)
  return parts.join(' ')
}
