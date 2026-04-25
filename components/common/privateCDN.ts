// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

const DEFAULT_MIMETYPE = 'image/jpg'

export async function fetchResource (url: string) {
  const response = await fetch(url, {
    // TODO(petemill): strip user-agent when this is possible
    // https://crbug.com/571722
    // TODO(petemill): Move this to c++ or a custom c++ privateCDNFetch
    // API so that we can strip 'dnt' header too.
    headers: new Headers({
      'Accept-Language': '*'
    })
  })
  return response
}

export async function getDataUrl (buffer: ArrayBuffer, mimeType = DEFAULT_MIMETYPE) {
  const unpaddedBlob = new Blob([buffer], { type: mimeType })
  const dataUrl = await new Promise<string>(resolve => {
    let reader = new FileReader()
    reader.onload = () => resolve(reader.result as string)
    reader.readAsDataURL(unpaddedBlob)
  })
  return dataUrl
}

export async function getUnpaddedAsDataUrl (buffer: ArrayBuffer, mimeType = DEFAULT_MIMETYPE) {
  const data = new DataView(buffer)
  const contentLength = data.getUint32(
    0,
    false /* big endian */
  )
  const unpaddedData = buffer.slice(4, contentLength + 4)
  return getDataUrl(unpaddedData, mimeType)
}
