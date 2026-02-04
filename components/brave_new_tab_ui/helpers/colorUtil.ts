// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/** enums for types of background color */
export enum ColorType {
  NONE,
  HEX,
  RGB,
  LINEAR_GRADIENT,
  RADIAL_GRADIENT
}

/** ColorData class takes a color string, parses it, and returns the luminance of the color */
export class ColorData {
  static #luminanceCache = new Map([[0 /* black */, 0.0], [255255255 /* white */, 1.0]])

  static clearCacheForTesting () { ColorData.#luminanceCache = new Map([[0 /* black */, 0.0], [255255255 /* white */, 1.0]]) }

  static rgbMatcher = /rgb\((\d+),\s*(\d+),\s*(\d+)\)/
  static hexMatcher = /#([0-9a-f]{6}|[0-9a-f]{3})/i

  type: ColorType
  value: string
  r: number
  g: number
  b: number

  constructor (value: string) {
    this.value = value

    const rgb = this.value.match(ColorData.rgbMatcher)
    if (rgb) {
      this.type = ColorType.RGB;
      [this.r, this.g, this.b] = [rgb[1], rgb[2], rgb[3]].map((v) => parseInt(v, 10))
      return
    }

    const hex = this.value.match(ColorData.hexMatcher)
    if (hex) {
      this.type = ColorType.HEX
      if (hex[0].length === 7) {
        [this.r, this.g, this.b] = [hex[1].substring(0, 2), hex[1].substring(2, 4), hex[1].substring(4, 6)].map((v) => parseInt(v, 16))
      } else if (hex[0].length === 4) {
        // short-hand hex
        [this.r, this.g, this.b] = [hex[1].substring(0, 1), hex[1].substring(1, 2), hex[1].substring(2, 3)].map((v) => parseInt(v + v, 16))
      }
      return
    }

    // TODO(sko) linear-gradient, radial-gradient
    this.type = ColorType.NONE
  }

  toString = () => {
    return `ColorData{${this.value}}`
  }

  isValid = () => {
    return this.type !== ColorType.NONE
  }

  get luminanceCacheKey () {
    return this.r * 1000000 + this.g * 1000 + this.b
  }

  get cachedLuminance () {
    return ColorData.#luminanceCache.get(this.luminanceCacheKey)
  }

  hasCachedLuminance = () => {
    return ColorData.#luminanceCache.has(this.luminanceCacheKey)
  }

  // Returns luminance of value of given |color| based on a definition from
  // https://www.w3.org/TR/WCAG20/#relativeluminancedef
  getRelativeLuminance = () => {
    if (!this.isValid()) {
        console.error(`Invalid color value: ${this}`)
        return 0.0
    }

    let r = this.r
    let g = this.g
    let b = this.b
    if (r === undefined || g === undefined || b === undefined) {
        console.error(`Couldn't extract RGB of: ${this}`)
        return 0.0
    }

    const cachedLuminance = this.cachedLuminance
    if (cachedLuminance) {
      return cachedLuminance
    }

    // Step 1. Lerp each channel to a value between 0 and 1.
    [r, g, b] = [r, g, b].map(v => v / 255);

    // Step 2. Adjust each channel's value.
    [r, g, b] = [r, g, b].map(v => {
        if (v <= 0.03928) {
            v /= 12.92
        } else {
            v = ((v + 0.055) / 1.055) ** 2.4
        }
        return v
    })

    // Step 3. Get Luminance.
    const cacheKey = this.luminanceCacheKey
    ColorData.#luminanceCache.set(cacheKey, 0.2126 * r + 0.7152 * g + 0.0722 * b)
    return ColorData.#luminanceCache.get(cacheKey)!
  }
}

/**
 * Returns contrast ratio of two colors based on a definition from
// https://www.w3.org/TR/WCAG20/#contrast-ratiodef
 * @param {ColorData} a - ColorData
 * @param {ColorData} b - ColorData
 * @returns Contrast ratio of two colors
 */
export function getContrastRatio (a: ColorData, b: ColorData): number {
  const [lighter, darker] = a.getRelativeLuminance() > b.getRelativeLuminance() ? [a, b] : [b, a]
  return (lighter.getRelativeLuminance() + 0.05) / (darker.getRelativeLuminance() + 0.05)
}

// This value is much lower than that of WCAG 2.0. https://www.w3.org/WAI/WCAG21/quickref/?versions=2.0#qr-visual-audio-contrast-contrast
// The value was decided by our UX team.
let thresholdForReadability = 1.5

export function setThresholdForReadability (threshold: number) {
  thresholdForReadability = threshold
}

export function getThresholdForReadability () {
  return thresholdForReadability
}

// Determines if . The threshold we use
/**
 * It returns true if |foreground| is readable on |background|.
 * @param {ColorData} background - The background color
 * @param {ColorData} foreground - The foreground color.
 * @returns {boolean} - True if the contrast ratio is greater than or equal to the threshold for readability
 */
export function isReadable (background: ColorData, foreground: ColorData) {
  return getContrastRatio(background, foreground) >= thresholdForReadability
}

/**
 * Convenient function to get readability for our background type
 * @param {NewTab.BackgroundWallpaper | undefined} background -
 * NewTab.BackgroundWallpaper | undefined
 * @returns {boolean} - true if background is readable
 */
export default function isReadableOnBackground (background: NewTab.BackgroundWallpaper | undefined) {
  if (!background || background.type !== 'color') {
    // Consider other cases readable. We can't determine the color from images.
    return true
  }

  const backgroundData = new ColorData(background.wallpaperColor)
  if (!backgroundData.isValid()) {
    console.error(`Invalid background color: ${backgroundData}`)
    return true
  }

  return isReadable(backgroundData, new ColorData('#fff')) &&
         isReadable(backgroundData, new ColorData('#FB542B'/* --interactive2 */)) &&
         isReadable(backgroundData, new ColorData('#A0A5EB'/* --interactive9 */))
}
