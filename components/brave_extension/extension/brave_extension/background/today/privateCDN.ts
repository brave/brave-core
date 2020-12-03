// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

export const URLS = {
  braveTodayFeed: 'https://brave-today-cdn.brave.com/brave-today/feed.json',
  braveTodayPublishers: 'https://brave-today-cdn.brave.com/sources.json'
}

export async function getUnpaddedAsDataUrl (buffer: ArrayBuffer, mimeType = 'image/jpg') {
  const data = new DataView(buffer)
  const contentLength = data.getUint32(
    0,
    false /* big endian */
  )
  const unpaddedData = buffer.slice(4, contentLength + 4)
  const unpaddedBlob = new Blob([unpaddedData], { type: mimeType })
  const dataUrl = await new Promise<string>(resolve => {
    let reader = new FileReader()
    reader.onload = () => resolve(reader.result as string)
    reader.readAsDataURL(unpaddedBlob)
  })
  return dataUrl
}
