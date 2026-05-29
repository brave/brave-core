// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  legacyTextSizeToVariant,
  resolveWalletTextVariant,
  walletFont,
  walletFontLineHeight,
} from './font.utils'

describe('legacyTextSizeToVariant', () => {
  it('maps heading sizes regardless of isBold', () => {
    expect(legacyTextSizeToVariant('28px')).toBe('heading.h1')
    expect(legacyTextSizeToVariant('28px', true)).toBe('heading.h1')
    expect(legacyTextSizeToVariant('22px')).toBe('heading.h2')
    expect(legacyTextSizeToVariant('20px')).toBe('heading.h3')
  })

  it('maps body sizes to regular variants by default', () => {
    expect(legacyTextSizeToVariant('18px')).toBe('large.regular')
    expect(legacyTextSizeToVariant('16px')).toBe('large.regular')
    expect(legacyTextSizeToVariant('14px')).toBe('default.regular')
    expect(legacyTextSizeToVariant('12px')).toBe('small.regular')
    expect(legacyTextSizeToVariant('11px')).toBe('xSmall.regular')
    expect(legacyTextSizeToVariant('10px')).toBe('xSmall.regular')
  })

  it('maps body sizes to semibold variants when isBold is true', () => {
    expect(legacyTextSizeToVariant('18px', true)).toBe('large.semibold')
    expect(legacyTextSizeToVariant('16px', true)).toBe('large.semibold')
    expect(legacyTextSizeToVariant('14px', true)).toBe('default.semibold')
    expect(legacyTextSizeToVariant('12px', true)).toBe('small.semibold')
    expect(legacyTextSizeToVariant('11px', true)).toBe('xSmall.semibold')
    expect(legacyTextSizeToVariant('10px', true)).toBe('xSmall.semibold')
  })

  it('falls back to large variants when textSize is undefined', () => {
    expect(legacyTextSizeToVariant(undefined)).toBe('large.regular')
    expect(legacyTextSizeToVariant(undefined, true)).toBe('large.semibold')
  })
})

describe('resolveWalletTextVariant', () => {
  it('prefers variant over legacy textSize', () => {
    expect(
      resolveWalletTextVariant({
        variant: 'small.regular',
        textSize: '28px',
      }),
    ).toBe('small.regular')
  })

  it('upgrades regular variants to semibold when isBold is true', () => {
    expect(
      resolveWalletTextVariant({
        variant: 'default.regular',
        isBold: true,
      }),
    ).toBe('default.semibold')
  })

  it('does not upgrade heading variants when isBold is true', () => {
    expect(
      resolveWalletTextVariant({
        variant: 'heading.h1',
        isBold: true,
      }),
    ).toBe('heading.h1')
  })

  it('leaves semibold variants unchanged when isBold is true', () => {
    expect(
      resolveWalletTextVariant({
        variant: 'default.semibold',
        isBold: true,
      }),
    ).toBe('default.semibold')
  })

  it('delegates to legacyTextSizeToVariant when variant is not provided', () => {
    expect(
      resolveWalletTextVariant({
        textSize: '14px',
        isBold: true,
      }),
    ).toBe('default.semibold')
  })
})

describe('walletFont helpers', () => {
  it('returns non-empty Leo font values for known variants', () => {
    const fontValue = walletFont('default.regular')
    expect(fontValue.length).toBeGreaterThan(0)
  })

  it('returns non-empty line heights for known variants', () => {
    const lineHeight = walletFontLineHeight('default.regular')
    expect(lineHeight.length).toBeGreaterThan(0)
  })
})
