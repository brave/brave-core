// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { css } from 'styled-components'
import { isRemoteImageURL } from './string-utils'

export const sizeCssValue = (size: string | number) => {
  return typeof size === 'number'
    ? `${size}px` // use pixels for numeric values
    : size
}

export const makeSizeMixin = (defaultSize: number | string) => css<{
  size?: number | string
}>`
  width: ${(p) => (p?.size ? sizeCssValue(p.size) : sizeCssValue(defaultSize))};

  height: ${(p) =>
    p?.size ? sizeCssValue(p.size) : sizeCssValue(defaultSize)};
`

export const makePaddingMixin = (defaultPadding: number | string) => css<{
  padding?: number | string
}>`
  padding: ${(p) =>
    p?.padding ? sizeCssValue(p.padding) : sizeCssValue(defaultPadding)};
`

// Pixels with max channel below this are treated as near-black and skipped
// when computing dominant color (so borders don't collapse to dark grays).
const DOMINANT_COLOR_NEAR_BLACK_MAX_CHANNEL = 40
// Pixels with min channel above this are treated as near-white and skipped
// (typical logo padding / backgrounds).
const DOMINANT_COLOR_NEAR_WHITE_MIN_CHANNEL = 235

const isNearBlackOrWhitePixel = (r: number, g: number, b: number) => {
  const hi = Math.max(r, g, b)
  const lo = Math.min(r, g, b)
  return (
    hi < DOMINANT_COLOR_NEAR_BLACK_MAX_CHANNEL
    || lo > DOMINANT_COLOR_NEAR_WHITE_MIN_CHANNEL
  )
}

const resolveWalletImageSrcForDominantColor = (src: string) =>
  isRemoteImageURL(src)
    ? `chrome://image?url=${encodeURIComponent(src)}&staticEncode=true`
    : src

/**
 * Samples a dominant RGBA color from an image that has finished loading
 * (naturalWidth / naturalHeight are set).
 */
export const getDominantColorFromLoadedImage = (
  img: HTMLImageElement,
  opacity: number = 0.2,
) => {
  const { naturalWidth, naturalHeight } = img

  // Higher number will have better precision but may be slower.
  const precision = 10

  // Calculate canvas sizes.
  const scaleRatio = Math.min(
    precision / naturalWidth,
    precision / naturalHeight,
  )
  const width = Math.ceil(naturalWidth * scaleRatio)
  const height = Math.ceil(naturalHeight * scaleRatio)
  const area = width * height

  // area might be 0.
  if (!area) {
    return undefined
  }
  // Construct canvas.
  let context = document.createElement('canvas').getContext('2d')

  // context may be null.
  if (!context) {
    return undefined
  }

  // Set canvase width and height.
  context.canvas.width = width
  context.canvas.height = height

  // Draw image to canvas.
  context.drawImage(img, 0, 0, width, height)

  // Get raw image data from canvas.
  const data = context.getImageData(0, 0, width, height).data
  let r = 0
  let g = 0
  let b = 0
  let counted = 0

  for (let i = 0; i < data.length; i += 4) {
    const pr = data[i]
    const pg = data[i + 1]
    const pb = data[i + 2]
    if (isNearBlackOrWhitePixel(pr, pg, pb)) {
      continue
    }
    r += pr
    g += pg
    b += pb
    counted++
  }

  // If everything was black/white padding, fall back to the full average.
  if (counted === 0) {
    for (let i = 0; i < data.length; i += 4) {
      r += data[i]
      g += data[i + 1]
      b += data[i + 2]
    }
    counted = area
  }

  r = ~~(r / counted)
  g = ~~(g / counted)
  b = ~~(b / counted)

  return `rgba(${r},${g},${b},${opacity})`
}

/**
 * Synchronous dominant color when the image is already decoded (e.g. cache hit).
 * Returns undefined if the image is not ready yet — use
 * {@link getDominantColorFromImageURLAsync} or {@link useDominantColorFromImageURL}
 * when you need a color after load.
 */
export const getDominantColorFromImageURL = (
  src: string,
  opacity: number = 0.2,
) => {
  if (!src) {
    return undefined
  }

  const imageSrc = resolveWalletImageSrcForDominantColor(src)
  const img = new Image()
  img.src = imageSrc

  if (img.complete && img.naturalWidth > 0) {
    return getDominantColorFromLoadedImage(img, opacity)
  }

  return undefined
}

export const getDominantColorFromImageURLAsync = (
  src: string,
  opacity: number = 0.2,
): Promise<string | undefined> => {
  if (!src) {
    return Promise.resolve(undefined)
  }

  const imageSrc = resolveWalletImageSrcForDominantColor(src)

  return new Promise((resolve) => {
    const img = new Image()
    const finish = () => resolve(getDominantColorFromLoadedImage(img, opacity))

    img.onload = finish
    img.onerror = () => resolve(undefined)
    img.src = imageSrc

    if (img.complete && img.naturalWidth > 0) {
      finish()
    }
  })
}
