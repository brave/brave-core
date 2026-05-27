// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { css } from 'styled-components'
import { font, typography } from '@brave/leo/tokens/css/variables'
import { sanitizeImageURL } from './string-utils'

export const systemFontStack = `system-ui, -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif`

export type WalletFontVariant =
  | 'default.regular'
  | 'default.semibold'
  | 'default.link'
  | 'small.regular'
  | 'small.semibold'
  | 'small.link'
  | 'large.regular'
  | 'large.semibold'
  | 'large.link'
  | 'xSmall.regular'
  | 'xSmall.semibold'
  | 'xSmall.link'
  | 'heading.h1'
  | 'heading.h2'
  | 'heading.h3'
  | 'heading.h4'
  | 'heading.display1'
  | 'heading.display2'
  | 'heading.display3'
  | 'components.label'
  | 'components.tableheader'
  | 'components.navbutton'
  | 'components.buttonSmall'
  | 'components.buttonDefault'
  | 'components.buttonLarge'
  | 'components.buttonJumbo'
  | 'components.numbersLarge'
  | 'monospace.large'
  | 'monospace.default'
  | 'monospace.small'

/** @deprecated Use WalletFontVariant instead */
export type LegacyTextSize =
  | '22px'
  | '20px'
  | '18px'
  | '16px'
  | '14px'
  | '12px'
  | '11px'
  | '10px'

const FONT_VARIANT_MAP: Record<WalletFontVariant, string> = {
  'default.regular': font.default.regular,
  'default.semibold': font.default.semibold,
  'default.link': font.default.link,
  'small.regular': font.small.regular,
  'small.semibold': font.small.semibold,
  'small.link': font.small.link,
  'large.regular': font.large.regular,
  'large.semibold': font.large.semibold,
  'large.link': font.large.link,
  'xSmall.regular': font.xSmall.regular,
  'xSmall.semibold': font.xSmall.semibold,
  'xSmall.link': font.xSmall.link,
  'heading.h1': font.heading.h1,
  'heading.h2': font.heading.h2,
  'heading.h3': font.heading.h3,
  'heading.h4': font.heading.h4,
  'heading.display1': font.heading.display1,
  'heading.display2': font.heading.display2,
  'heading.display3': font.heading.display3,
  'components.label': font.components.label,
  'components.tableheader': font.components.tableheader,
  'components.navbutton': font.components.navbutton,
  'components.buttonSmall': font.components.buttonSmall,
  'components.buttonDefault': font.components.buttonDefault,
  'components.buttonLarge': font.components.buttonLarge,
  'components.buttonJumbo': font.components.buttonJumbo,
  'components.numbersLarge': font.components.numbersLarge,
  'monospace.large': font.monospace.large,
  'monospace.default': font.monospace.default,
  'monospace.small': font.monospace.small,
}

export const walletFont = (variant: WalletFontVariant) =>
  FONT_VARIANT_MAP[variant]

const TYPOGRAPHY_LINE_HEIGHT_MAP: Record<WalletFontVariant, string> = {
  'default.regular': typography.default.regular.lineHeight,
  'default.semibold': typography.default.semibold.lineHeight,
  'default.link': typography.default.link.lineHeight,
  'small.regular': typography.small.regular.lineHeight,
  'small.semibold': typography.small.semibold.lineHeight,
  'small.link': typography.small.link.lineHeight,
  'large.regular': typography.large.regular.lineHeight,
  'large.semibold': typography.large.semibold.lineHeight,
  'large.link': typography.large.link.lineHeight,
  'xSmall.regular': typography.xSmall.regular.lineHeight,
  'xSmall.semibold': typography.xSmall.semibold.lineHeight,
  'xSmall.link': typography.xSmall.link.lineHeight,
  'heading.h1': typography.heading.h1.lineHeight,
  'heading.h2': typography.heading.h2.lineHeight,
  'heading.h3': typography.heading.h3.lineHeight,
  'heading.h4': typography.heading.h4.lineHeight,
  'heading.display1': typography.heading.display1.lineHeight,
  'heading.display2': typography.heading.display2.lineHeight,
  'heading.display3': typography.heading.display3.lineHeight,
  'components.label': typography.components.label.lineHeight,
  'components.tableheader': typography.components.tableheader.lineHeight,
  'components.navbutton': typography.components.navbutton.lineHeight,
  'components.buttonSmall': typography.components.buttonSmall.lineHeight,
  'components.buttonDefault': typography.components.buttonDefault.lineHeight,
  'components.buttonLarge': typography.components.buttonLarge.lineHeight,
  'components.buttonJumbo': typography.components.buttonJumbo.lineHeight,
  'components.numbersLarge': typography.components.numbersLarge.lineHeight,
  'monospace.large': typography.monospace.large.lineHeight,
  'monospace.default': typography.monospace.default.lineHeight,
  'monospace.small': typography.monospace.small.lineHeight,
}

export const walletFontLineHeight = (variant: WalletFontVariant) =>
  TYPOGRAPHY_LINE_HEIGHT_MAP[variant]

export const legacyTextSizeToVariant = (
  textSize: LegacyTextSize | undefined,
  isBold?: boolean,
): WalletFontVariant => {
  switch (textSize) {
    case '22px':
      return 'heading.h2'
    case '20px':
      return 'heading.h3'
    case '18px':
      return isBold ? 'large.semibold' : 'large.regular'
    case '16px':
      return isBold ? 'large.semibold' : 'large.regular'
    case '14px':
      return isBold ? 'default.semibold' : 'default.regular'
    case '12px':
      return isBold ? 'small.semibold' : 'small.regular'
    case '11px':
    case '10px':
      return isBold ? 'xSmall.semibold' : 'xSmall.regular'
    default:
      return isBold ? 'large.semibold' : 'large.regular'
  }
}

export const resolveWalletTextVariant = (props: {
  variant?: WalletFontVariant
  textSize?: LegacyTextSize
  isBold?: boolean
}): WalletFontVariant => {
  if (props.variant) {
    if (
      props.isBold
      && !props.variant.includes('semibold')
      && !props.variant.startsWith('heading.')
    ) {
      const semiboldVariant = props.variant.replace(
        '.regular',
        '.semibold',
      ) as WalletFontVariant
      if (semiboldVariant in FONT_VARIANT_MAP) {
        return semiboldVariant
      }
    }
    return props.variant
  }
  return legacyTextSizeToVariant(props.textSize, props.isBold)
}

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
  sanitizeImageURL(src)

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
