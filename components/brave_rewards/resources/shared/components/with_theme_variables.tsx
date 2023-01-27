// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'
import { DarkColorSchemeContext } from '$web-common/BraveCoreThemeProvider'
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

/**
 * Deprecated - Instead use @brave/leo css variables or styled-component theme.
 *
 * Converts BraveUI styled-component theme to css variables
 */
export function WithThemeVariables (props: { children: React.ReactNode }) {
  const isDarkMode = React.useContext(DarkColorSchemeContext)

  const currentTheme = isDarkMode ? 'dark' : 'default'

  return (
    <Wrapper className={`brave-theme-${currentTheme}`}>
      {props.children}
    </Wrapper>
  )
}
