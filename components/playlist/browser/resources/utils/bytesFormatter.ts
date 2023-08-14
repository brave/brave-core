// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { PlaylistItem } from 'gen/brave/components/playlist/common/mojom/playlist.mojom.m.js'

// Note that we hardly expect files to be bigger than 1TB. Generally, 4k 1 hour long
// video is 30GB more or less. So it's unlikely for the |bytes| to exceed
// the number range. Roughly we can represents more than 10000 videos in that
// size with number type.
// * Number.MAX_SAFE_INTEGER == 9007199254740991
// *  30 GB                  == 30000000000
export function formatBytes (bytes: bigint) {
  // Here, we convert the bytes into number first so that we can
  // get digits in decimal places.
  const mb = Number(bytes) / (1 << 20)

  const toFixedDigit = (num: number) => {
    // We're trying to keep 3 digits
    if (num >= 100) {
      return Math.round(num) // NNN
    }

    if (num >= 10) {
      return num.toFixed(1) // NN.N
    }

    return num.toFixed(2) // N.NN
  }

  if (mb < 1024) {
    return toFixedDigit(mb) + ' MB'
  }

  return toFixedDigit(mb / 1024) + ' GB'
}

export function getFormattedTotalBytes (item: PlaylistItem[]) {
  return formatBytes(
    item.reduce((sum, item) => {
      return sum + item.mediaFileBytes
    }, BigInt(0))
  )
}
