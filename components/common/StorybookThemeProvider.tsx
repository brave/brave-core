// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { select } from '@storybook/addon-knobs'
import ITheme from 'brave-ui/theme/theme-interface'
import Theme from 'brave-ui/theme/brave-default'
import DarkTheme from 'brave-ui/theme/brave-dark'
import BraveCoreThemeProvider from './BraveCoreThemeProvider'

type Props = {
  darkTheme?: ITheme
  lightTheme?: ITheme
}

const ThemeProvider: React.FunctionComponent<Props> = (props) => {
  const themeName = select(
    'Theme',
    { 'Light': 'Light', 'Dark': 'Dark' },
    'Light'
  )
  let darkTheme: ITheme = props.darkTheme as ITheme || DarkTheme
  let lightTheme: ITheme = props.lightTheme as ITheme || Theme
  return (
    <BraveCoreThemeProvider
      dark={darkTheme}
      light={lightTheme}
      initialThemeType={themeName as chrome.braveTheme.ThemeType}
    >
    {props.children}
    </BraveCoreThemeProvider>
  )
}

export default ThemeProvider
