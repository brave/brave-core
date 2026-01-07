/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import { skColorToRgba } from 'chrome://resources/js/color_utils.js'

import { SkColor } from '../api/welcome_api'
import { useWelcomeApi } from '../api/welcome_api_context'
import { getString } from '../lib/strings'
import * as themeColors from '../lib/theme_colors'
import { ThemeColorSwatch } from './theme_color_swatch'

import { style } from './theme_color_selector.style'

export function ThemeColorSelector() {
  const api = useWelcomeApi()

  const theme = api.useTheme().data
  const isDarkMode = theme?.isDarkMode ?? false
  const chromeColors = api.useGetChromeColorsData(isDarkMode)
  const greyBaseline = themeColors.getGreyBaselineColors(isDarkMode)

  if (!theme) {
    return null
  }

  return (
    <div data-css-scope={style.scope}>
      <ColorSwatchButton
        colors={greyBaseline}
        name={getString('WELCOME_PAGE_THEME_GREY_COLOR_LABEL')}
        selected={themeColors.themeIsGrey(theme)}
        onClick={() => api.setGreyThemeColor()}
      />
      {chromeColors.map((color) => (
        <ColorSwatchButton
          key={color.name}
          colors={color}
          name={color.name}
          selected={themeColors.themeMatchesColor(theme, color)}
          onClick={() => api.setThemeColor(color.seed, color.variant)}
        />
      ))}
    </div>
  )
}

interface SwatchColors {
  foreground: SkColor
  background: SkColor
  base: SkColor
}

interface ColorSwatchButtonProps {
  colors: SwatchColors
  name: string
  selected: boolean
  onClick: () => void
}

function ColorSwatchButton(props: ColorSwatchButtonProps) {
  return (
    <button
      className={`color-swatch${props.selected ? ' selected' : ''}`}
      aria-pressed={props.selected}
      title={props.name}
      onClick={props.onClick}
    >
      <ThemeColorSwatch
        foregroundColor={skColorToRgba(props.colors.foreground)}
        backgroundColor={skColorToRgba(props.colors.background)}
        baseColor={skColorToRgba(props.colors.base)}
      />
      {props.selected && (
        <Icon
          className='checkmark'
          name='check-circle-filled'
        />
      )}
    </button>
  )
}
