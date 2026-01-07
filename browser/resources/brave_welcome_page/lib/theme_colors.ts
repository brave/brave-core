/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { ChromeColor, SkColor, Theme } from '../api/welcome_api'

interface SwatchColors {
  foreground: SkColor
  background: SkColor
  base: SkColor
}

// Grey baseline palette shown when no Chrome color is applied. Brave only
// exposes the grey baseline (the default-color swatch is hidden), mirroring the
// theme color picker in the sidepanel. Values match the upstream constants.
const greyBaselineLight: SwatchColors = {
  foreground: { value: 0xffe3e3e3 },
  background: { value: 0xff0b57d0 },
  base: { value: 0xffc7c7c7 },
}

const greyBaselineDark: SwatchColors = {
  foreground: { value: 0xff474747 },
  background: { value: 0xffa8c7fa },
  base: { value: 0xff757575 },
}

export function getGreyBaselineColors(dark: boolean) {
  return dark ? greyBaselineDark : greyBaselineLight
}

export function themeIsGrey(theme: Theme) {
  return !theme.followDeviceTheme && theme.isGreyBaseline
}

export function themeMatchesColor(theme: Theme, color: ChromeColor) {
  return (
    !theme.followDeviceTheme
    && !theme.isGreyBaseline
    && !!theme.foregroundColor
    && theme.seedColor.value === color.seed.value
    && theme.browserColorVariant === color.variant
  )
}
