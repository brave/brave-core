/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
const qr = require('qr-image')

export const generateQRData = (url: string, asset: string, qrAction: any) => {
  const image = qr.image(url)
  try {
    let chunks: Array<Uint8Array> = []
    image
      .on('data', (chunk: Uint8Array) => chunks.push(chunk))
      .on('end', () => {
        qrAction(asset, `data:image/png;base64,${Buffer.concat(chunks).toString('base64')}`)
      })
  } catch (error) {
    console.error('Could not create deposit QR', error.toString())
  }
}

export const getUSDPrice = (accountBTCBalance: string, btcUSDPrice: string) => {
  if (!accountBTCBalance || !btcUSDPrice) {
    return '0.00'
  }

  const btcUSDPriceNumber = parseFloat(btcUSDPrice)
  const btcBalanceNumber = parseFloat(accountBTCBalance)

  if (isNaN(btcUSDPriceNumber) || isNaN(btcBalanceNumber)) {
    return '0.00'
  }

  return (btcUSDPriceNumber * btcBalanceNumber).toFixed(2)
}
