// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as qr from 'qr-image'

export const generateQRCode = (data: string): Promise<string> => {
  return new Promise((resolve, reject) => {
    const image = qr.image(data)
    let chunks: Uint8Array[] = []
    image
      .on('data', (chunk: Uint8Array) => chunks.push(chunk))
      .on('end', () => {
        resolve(`data:image/png;base64,${Buffer.concat(chunks).toString('base64')}`)
      })
  })
}
