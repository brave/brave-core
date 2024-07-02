// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import 'emptykit.css'
import * as React from 'react'
import { withKnobs, boolean } from '@storybook/addon-knobs'
import { setIconBasePath } from '@brave/leo/react/icon'
import '../components/web-components/app.global.scss'
import { getString } from './locale'
import ThemeProvider from '../components/common/BraveCoreThemeProvider'

// Fonts
import '../ui/webui/resources/fonts/poppins.css'
import '../ui/webui/resources/fonts/manrope.css'
import '../ui/webui/resources/fonts/inter.css'

// Icon path
// The storybook might be hosted at the root, but it might also be hosted
// somewhere deep. The icons will be hosted in the relative path of the
// storybook. Let's find the relative path we're at, and give that to
// Nala icons.
if (!document.location.pathname.endsWith('/iframe.html')) {
  // Perhaps storybook was upgraded and this changed?
  console.error('Could not ascertain path that the storybook is hosted at. Not able to set static icon path!')
} else {
  const storybookPath = document.location.pathname.substring(0, document.location.pathname.lastIndexOf('/'))
  setIconBasePath(`${storybookPath}/icons`)
}

export const parameters = {
  backgrounds: {
    default: 'Dynamic',
    values: [
      { name: 'Dynamic', value: 'var(--background1)' },
      { name: 'Neutral300', value: '#DEE2E6' },
      { name: 'Grey700', value: '#5E6175' },
      { name: 'White', value: '#FFF' },
      { name: 'Grey900', value: '#1E2029' }
    ]
  }
};

const global: any = window
global.loadTimeData = {
  getString,
  getBoolean(key: string) {
    return false
  },
  getInteger(key: string) {
    return 0
  }
}

if (!global.chrome) global.chrome = { extension: {} }
global.chrome.extension = {
  inIncognitoContext: false
}

export default {
  decorators: [
    (Story: () => JSX.Element) => (
      <div dir={boolean('rtl?', false) ? 'rtl' : ''}>
        <Story />
      </div>
    ),
    (Story: () => JSX.Element, context: any) => (
      <ThemeProvider
        dark={context.args.darkTheme}
        light={context.args.lightTheme}
      >
        <Story />
      </ThemeProvider>
    ),
    withKnobs
  ]
}
