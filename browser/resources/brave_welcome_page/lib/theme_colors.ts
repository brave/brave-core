/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { ChromeColor, Theme } from '../api/welcome_api'

import {
  LIGHT_BASELINE_GREY_COLOR,
  DARK_BASELINE_GREY_COLOR,
} from '../../../../../ui/webui/resources/cr_components/theme_color_picker/color_utils'

export function getGreyBaselineColors(dark: boolean) {
  return dark ? DARK_BASELINE_GREY_COLOR : LIGHT_BASELINE_GREY_COLOR
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
