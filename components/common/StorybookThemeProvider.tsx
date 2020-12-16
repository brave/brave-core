// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { select } from '@storybook/addon-knobs'
import Theme from 'brave-ui/theme/brave-default'
import DarkTheme from 'brave-ui/theme/brave-dark'
import BraveCoreThemeProvider from './BraveCoreThemeProvider'

const ThemeProvider: React.FunctionComponent = ({ children }) => {
  const themeName = select(
    'Theme',
    { ['Light']: 'Light', ['Dark']: 'Dark' },
    'Light'
  )
  console.log('theme', themeName)
  return (
    <BraveCoreThemeProvider
      dark={DarkTheme}
      light={Theme}
      initialThemeType={themeName as chrome.braveTheme.ThemeType}
    >
    {children}
    </BraveCoreThemeProvider>
  )
}

export default ThemeProvider
