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
  const intArr = seed.split(',').map(Number)

  const toHexString = (intArr: ArrayLike<number>) => {
    return Array
    .from(new Uint8Array(intArr))
    .map(b => b.toString(16).padStart(2, '0'))
    .join('')
  }

  const hexString = toHexString(intArr)
  const image = qr.image(hexString)

  try {
    let chunks: Array<Uint8Array> = []
    image
      .on('data', (chunk: Uint8Array) => chunks.push(chunk))
      .on('end', () => {
        const base64Image = `data:image/png;base64,${Buffer.concat(chunks).toString('base64')}`
        actions.onGenerateQRCodeImageSource(base64Image)
      })
  } catch (error) {
    console.error('[SYNC] QR image error:', error.toString())
  }
}

/**
 * Gets the user's operating system name to ne ised as a default device name
 * @returns {string} - A string including the OS + the desktop suffix
 */
export const getDefaultDeviceName = () => {
  const userAgent = window.navigator.userAgent
  const currentPlatform = window.navigator.platform
  const macosVariants = ['Macintosh', 'MacIntel', 'MacPPC', 'Mac68K']
  const windowsVariants = ['Win32', 'Win64', 'Windows', 'WinCE']
  const iOSVariants = ['iPhone', 'iPad', 'iPod']
  const androidVariants = ['Android']

  let system

  if (macosVariants.includes(currentPlatform)) {
    system = 'Mac'
  } else if (windowsVariants.includes(currentPlatform)) {
    system = 'Windows'
  } else if (iOSVariants.includes(currentPlatform)) {
    system = 'iOS'
  } else if (androidVariants.includes(userAgent)) {
    system = 'Android'
  } else {
    system = 'Linux'
  }
  return `${system} Desktop`
}
