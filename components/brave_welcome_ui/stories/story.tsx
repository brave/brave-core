/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import welcomeDarkTheme from '../theme/welcome-dark'
import welcomeLightTheme from '../theme/welcome-light'
import { boolean } from '@storybook/addon-knobs'

// Components
import WelcomePage from './page/index'

export default {
  title: 'Welcome',
  args: {
    darkTheme: welcomeDarkTheme,
    lightTheme: welcomeLightTheme
  },
  parameters: {
    layout: 'fullscreen'
  }
}

export const Page = () => (
  <WelcomePage isDefaultSearchGoogle={boolean('Is default search google?', true)}/>
)
