// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as ColorUtil from './colorUtil'

describe('ColorData', () => {
  describe('constructor', () => {
    it('can be constructed from HEX color', () => {
      const color = new ColorUtil.ColorData('#123456')
      expect(color.type).toBe(ColorUtil.ColorType.HEX)
      expect(color.r).toBe(0x12)
      expect(color.g).toBe(0x34)
      expect(color.b).toBe(0x56)
    })

    it('can be constructed from HEX shorthand color', () => {
      const color = new ColorUtil.ColorData('#123')
      expect(color.type).toBe(ColorUtil.ColorType.HEX)
      expect(color.r).toBe(0x11)
      expect(color.g).toBe(0x22)
      expect(color.b).toBe(0x33)
    })

    it('can be constructed from rgb()', () => {
      let color = new ColorUtil.ColorData('rgb(1, 2, 3)')
      expect(color.type).toBe(ColorUtil.ColorType.RGB)
      expect(color.r).toBe(1)
      expect(color.g).toBe(2)
      expect(color.b).toBe(3)

      color = new ColorUtil.ColorData('rgb(0, 0, 0)')
      expect(color.type).toBe(ColorUtil.ColorType.RGB)
      expect(color.r).toBe(0)
      expect(color.g).toBe(0)
      expect(color.b).toBe(0)

      color = new ColorUtil.ColorData('rgb(255, 255, 255)')
      expect(color.type).toBe(ColorUtil.ColorType.RGB)
      expect(color.r).toBe(255)
      expect(color.g).toBe(255)
      expect(color.b).toBe(255)
    })
  })

  describe('getRelativeLuminance()', () => {
    // Can cross check values from https://planetcalc.com/7779/?color=%23FFFFFF
    // (modify |color| query value)
    it('returns 0.0 for black', () => { expect(new ColorUtil.ColorData('rgb(0, 0, 0)').getRelativeLuminance()).toBe(0) })
    it('returns 1.0 for white', () => { expect(new ColorUtil.ColorData('rgb(255, 255, 255)').getRelativeLuminance()).toBe(1) })
    it('returns 0.2126 for red', () => { expect(new ColorUtil.ColorData('rgb(255, 0, 0)').getRelativeLuminance()).toBe(0.2126) })
    it('returns 0.7152 for green', () => { expect(new ColorUtil.ColorData('rgb(0, 255, 0)').getRelativeLuminance()).toBe(0.7152) })
    it('returns 0.0722 for blue', () => { expect(new ColorUtil.ColorData('rgb(0, 0, 255)').getRelativeLuminance()).toBe(0.0722) })
  })

  describe('luminanceCache', () => {
    beforeEach(() => { ColorUtil.ColorData.clearCacheForTesting() })

    it('has L for rgb(255, 255, 255) in the beginning', () => { expect(new ColorUtil.ColorData('rgb(255, 255, 255)').hasCachedLuminance()).toBeTruthy() })
    it('has L for #ffffff in the beginning', () => { expect(new ColorUtil.ColorData('#ffffff').hasCachedLuminance()).toBeTruthy() })
    it('has L for rgb(0, 0, 0) in the beginning', () => { expect(new ColorUtil.ColorData('rgb(0, 0, 0)').hasCachedLuminance()).toBeTruthy() })
    it('has L for #000 in the beginning', () => { expect(new ColorUtil.ColorData('#000').hasCachedLuminance()).toBeTruthy() })
    it('caches L once a L is calculated', () => {
        const color = new ColorUtil.ColorData('rgb(255, 0, 0)')
        expect(color.hasCachedLuminance()).toBeFalsy()

        // Calculate L and make cache hot
        color.getRelativeLuminance()
        expect(color.hasCachedLuminance()).toBeTruthy()

        // Equivalent color should use the same cache
        expect(new ColorUtil.ColorData('#ff0000').hasCachedLuminance).toBeTruthy()
    })
  })
})

describe('getContrastRatio()', () => {
  // Can cross check values from https://planetcalc.com/7779/?color1=%23FFFFFF&color2=%23000000
  // (modify |color1| and |color2| query value)
  it('returns 1.0 for same color', () => {
    expect(ColorUtil.getContrastRatio(new ColorUtil.ColorData('rgb(0, 0, 0)'),
                                      new ColorUtil.ColorData('#000'))).toBe(1)
  })

  it('returns 21 for black and white', () => {
    expect(ColorUtil.getContrastRatio(new ColorUtil.ColorData('#fff'),
                                      new ColorUtil.ColorData('#000'))).toBe(21)
  })

  it('returns 2.91 for red and green', () => {
    expect(ColorUtil.getContrastRatio(new ColorUtil.ColorData('#f00'),
                                      new ColorUtil.ColorData('#0f0'))).toBe(2.9139375476009137)
  })

  it('returns 6.26 for green and blue', () => {
    expect(ColorUtil.getContrastRatio(new ColorUtil.ColorData('#0f0'),
                                      new ColorUtil.ColorData('#00f'))).toBe(6.261865793780687)
  })

  it('returns 2.14 for blue and red', () => {
    expect(ColorUtil.getContrastRatio(new ColorUtil.ColorData('#00f'),
                                      new ColorUtil.ColorData('#f00'))).toBe(2.148936170212766)
  })
})

describe('isReadable()', () => {
  describe('When background is black, stats are visible', () => {
    const background = new ColorUtil.ColorData('#000')
    it('returns true for tracker stat color', () => { expect(ColorUtil.isReadable(background, new ColorUtil.ColorData('#FB542B'/* --interactive2 */))).toBeTruthy() })
    it('returns true for bandwidth saved stat color', () => { expect(ColorUtil.isReadable(background, new ColorUtil.ColorData('#A0A5EB'/* --interactive9 */))).toBeTruthy() })
    it('returns true for time saved stat color', () => { expect(ColorUtil.isReadable(background, new ColorUtil.ColorData('#FFFFFF'))).toBeTruthy() })
  })

  describe('When background is #F0CB44, stats are not visible', () => {
    const background = new ColorUtil.ColorData('#F0CB44')
    it('returns true for tracker stat color', () => { expect(ColorUtil.isReadable(background, new ColorUtil.ColorData('#FB542B'/* --interactive2 */))).toBeTruthy() })
    it('returns false for bandwidth saved stat color', () => { expect(ColorUtil.isReadable(background, new ColorUtil.ColorData('#A0A5EB'/* --interactive9 */))).toBeFalsy() })
    it('returns true for time saved stat color', () => { expect(ColorUtil.isReadable(background, new ColorUtil.ColorData('#FFFFFF'))).toBeTruthy() })
  })

  describe('When background is #2197F9 stats are not visible', () => {
    const background = new ColorUtil.ColorData('#2197F9')
    it('returns false for tracker stat color', () => { expect(ColorUtil.isReadable(background, new ColorUtil.ColorData('#FB542B'/* --interactive2 */))).toBeFalsy() })
    it('returns false for bandwidth saved stat color', () => { expect(ColorUtil.isReadable(background, new ColorUtil.ColorData('#A0A5EB'/* --interactive9 */))).toBeFalsy() })
    it('returns true for time saved stat color', () => { expect(ColorUtil.isReadable(background, new ColorUtil.ColorData('#FFFFFF'))).toBeTruthy() })
  })
})
