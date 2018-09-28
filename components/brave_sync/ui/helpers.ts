/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const qr = require('qr-image')

export let actions: any = null

export const getActions = () => actions
export const setActions = (newActions: any) => actions = newActions

/**
 * Generates the QR code image source based on sync seed
 * @param {string} seed - the seed received by the sync back-end
 */
export const generateQRCodeImageSource = (seed: string) => {
  const image = qr.image(Buffer.from(seed).toString('hex'))
  try {
    let chunks: Array<Uint8Array> = []
    image
      .on('data', (chunk: Uint8Array) => chunks.push(chunk))
      .on('end', () => {
        const base64Image = 'data:image/png;base64,' + Buffer.concat(chunks).toString('base64')
        actions.onGenerateQRCodeImageSource(base64Image)
      })
  } catch (error) {
    console.error('[SYNC] QR image error:', error.toString())
  }
}
