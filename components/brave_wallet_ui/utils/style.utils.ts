// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { css } from 'styled-components'

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

export const getDominantColorFromImageURL = (src: string) => {
  if (!src) {
    return undefined
  }

  try {
    // Construct new image from src.
    let img = new Image()
    img.src = src

    // Images actual width and height.
    const { naturalWidth, naturalHeight } = img

    // Higher number will have better precision but may be slower.
    const precision = 10

    // Calculate canvas sizes.
    const scaleRatio = Math.min(
      precision / naturalWidth,
      precision / naturalHeight
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

    // Compare and find average color values
    for (let i = 0; i < data.length; i += 4) {
      r += data[i]
      g += data[i + 1]
      b += data[i + 2]
    }

    r = ~~(r / area)
    g = ~~(g / area)
    b = ~~(b / area)

    return `rgba(${r},${g},${b},0.2)`
  } catch {
    return undefined
  }
}