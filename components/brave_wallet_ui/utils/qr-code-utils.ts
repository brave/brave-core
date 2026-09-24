// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as qr from 'qr-image'

export const generateQRCode = (data: string): Promise<string> => {
  return new Promise((resolve, reject) => {
    try {
      // Use SVG output instead of PNG to avoid zlib dependency issues
      // with the browser-assert polyfill (zlib requires assert.ok which
      // browser-assert doesn't provide)
      // This is only for generating QR codes with trusted data
      const image = qr.image(data, { type: 'svg' })
      let chunks: string[] = []
      image
        .on('data', (chunk: Buffer) => chunks.push(chunk.toString()))
        .on('end', () => {
          const svgString = chunks.join('')
          resolve(`data:image/svg+xml;base64,${btoa(svgString)}`)
        })
        .on('error', (error: Error) => {
          reject(error)
        })
    } catch (error) {
      reject(error)
    }
  })
}
