/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import defaultTheme from 'brave-ui/theme/brave-default'

export function injectThemeVariables (element: HTMLElement) {
  for (const [key, value] of Object.entries(defaultTheme.color)) {
    element.style.setProperty(`--brave-color-${key}`, String(value))
  }
  for (const [key, value] of Object.entries(defaultTheme.palette)) {
    element.style.setProperty(`--brave-palette-${key}`, String(value))
  }
  for (const [key, value] of Object.entries(defaultTheme.fontFamily)) {
    element.style.setProperty(`--brave-font-${key}`, String(value))
  }
}
