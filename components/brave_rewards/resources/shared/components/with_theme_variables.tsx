/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styled, { ThemeContext } from 'styled-components'
import braveDefaultTheme from 'brave-ui/theme/brave-default'
import braveDarkTheme from 'brave-ui/theme/brave-dark'

function createThemeRules (theme: any) {
  if (!theme) {
    return ''
  }

  let list = []

  for (const [key, value] of Object.entries(theme.color)) {
    list.push(`--brave-color-${key}: ${String(value)};`)
  }
  for (const [key, value] of Object.entries(theme.palette)) {
    list.push(`--brave-palette-${key}: ${String(value)};`)
  }
  for (const [key, value] of Object.entries(theme.fontFamily)) {
    list.push(`--brave-font-${key}: ${String(value)};`)
  }

  return list.join('\n')
}

const Wrapper = styled.div`
  ${createThemeRules(braveDefaultTheme)}

  &.brave-theme-dark {
    ${createThemeRules(braveDarkTheme)}
  }
`

function normalizeThemeName (name: string) {
  if (name.toLowerCase() === 'dark' || name === braveDarkTheme.name) {
    return 'dark'
  }
  return 'default'
}

export function WithThemeVariables (props: { children: React.ReactNode }) {
  const styledComponentsTheme = React.useContext(ThemeContext) || {}
  const [themeName, setThemeName] = React.useState('')

  React.useEffect(() => {
    if (chrome && chrome.braveTheme) {
      chrome.braveTheme.getBraveThemeType(setThemeName)
      chrome.braveTheme.onBraveThemeTypeChanged.addListener(setThemeName)
    }
  }, [])

  const currentTheme = normalizeThemeName(
    themeName || styledComponentsTheme.name || '')

  return (
    <Wrapper className={`brave-theme-${currentTheme}`}>
      {props.children}
    </Wrapper>
  )
}
