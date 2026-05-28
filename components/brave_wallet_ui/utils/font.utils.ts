// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { font, typography } from '@brave/leo/tokens/css/variables'

export const walletFontFamily =
  "system-ui, -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif"

type ExcludeToString<T> = {
  [K in keyof T as K extends 'toString' ? never : K]: T[K]
}

type LeoFontLeafPaths<T, Prefix extends string = ''> = {
  [K in keyof ExcludeToString<T>]: ExcludeToString<T>[K] extends string
    ? `${Prefix}${K & string}`
    : ExcludeToString<T>[K] extends Record<string, unknown>
      ? LeoFontLeafPaths<ExcludeToString<T>[K], `${Prefix}${K & string}.`>
      : never
}[keyof ExcludeToString<T>]

export type WalletFontVariant = LeoFontLeafPaths<typeof font>

/** @deprecated Use WalletFontVariant instead */
export type LegacyTextSize =
  | '28px'
  | '22px'
  | '20px'
  | '18px'
  | '16px'
  | '14px'
  | '12px'
  | '11px'
  | '10px'

const parseWalletFontVariant = (variant: WalletFontVariant) => {
  const separatorIndex = variant.indexOf('.')
  return {
    group: variant.slice(0, separatorIndex),
    name: variant.slice(separatorIndex + 1),
  }
}

type LeoFontTokens = ExcludeToString<typeof font>
type LeoTypographyTokens = ExcludeToString<typeof typography>

const isWalletFontVariant = (variant: string): variant is WalletFontVariant => {
  const separatorIndex = variant.indexOf('.')
  if (separatorIndex <= 0) {
    return false
  }

  const group = variant.slice(0, separatorIndex)
  const name = variant.slice(separatorIndex + 1)
  if (!(group in font)) {
    return false
  }

  const groupFont = font[group as keyof typeof font]
  return (
    typeof groupFont === 'object' && groupFont !== null && name in groupFont
  )
}

export const walletFont = (variant: WalletFontVariant): string => {
  const { group, name } = parseWalletFontVariant(variant)
  type LeoFontGroup = LeoFontTokens[keyof LeoFontTokens]
  const groupTokens = font[
    group as keyof LeoFontTokens
  ] as ExcludeToString<LeoFontGroup>
  return groupTokens[name as keyof typeof groupTokens]
}

export const walletFontLineHeight = (variant: WalletFontVariant) => {
  const { group, name } = parseWalletFontVariant(variant)
  type LeoTypographyGroup = LeoTypographyTokens[keyof LeoTypographyTokens]
  const groupTokens = typography[
    group as keyof LeoTypographyTokens
  ] as ExcludeToString<LeoTypographyGroup>
  const token = groupTokens[name as keyof typeof groupTokens] as {
    lineHeight: string
  }
  return token.lineHeight
}

export const legacyTextSizeToVariant = (
  textSize: LegacyTextSize | undefined,
  isBold?: boolean,
): WalletFontVariant => {
  switch (textSize) {
    case '28px':
      return 'heading.h1'
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
      if (isWalletFontVariant(semiboldVariant)) {
        return semiboldVariant
      }
    }
    return props.variant
  }
  return legacyTextSizeToVariant(props.textSize, props.isBold)
}
