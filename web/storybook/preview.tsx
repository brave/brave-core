// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import 'emptykit.css'
import * as React from 'react'
import { withKnobs, boolean } from '@storybook/addon-knobs'
import { setIconBasePath } from '@brave/leo/react/icon'
import { getString } from './locale'
import ThemeProvider from '$web-common/BraveCoreThemeProvider'
import StyledComponentsProvider from '$web-common/StyledComponentsProvider'

// Nala design tokens (the `--leo-*` custom properties). In the browser these
// come from `chrome://resources/brave/css/nala.css`, which Storybook can't
// load, so pull in the static token stylesheet globally here. It defines both
// the light and dark values, keyed off `prefers-color-scheme`.
import '@brave/leo/tokens/css/variables.css'

// Fonts
import '../../ui/webui/resources/fonts/poppins.css'
import '../../ui/webui/resources/fonts/manrope.css'
import '../../ui/webui/resources/fonts/inter.css'

// Icon path
// The storybook might be hosted at the root, but it might also be hosted
// somewhere deep. The icons will be hosted in the relative path of the
// storybook. Let's find the relative path we're at, and give that to
// Nala icons.
//
// Note: the base path is global and is applied retroactively to every mounted
// icon, so the *last* caller wins. Page entry points set it to a `chrome://`
// path that can't be loaded on the web, so they must only do so when actually
// mounting the page - never at module scope, where merely importing them from a
// story would clobber this.
if (!document.location.pathname.endsWith('/iframe.html')) {
  // Perhaps storybook was upgraded and this changed?
  console.error(
    'Could not ascertain path that the storybook is hosted at. Not able to set static icon path!',
  )
} else {
  const storybookPath = document.location.pathname.substring(
    0,
    document.location.pathname.lastIndexOf('/'),
  )
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
      { name: 'Grey900', value: '#1E2029' },
    ],
  },
}

const global: any = window
global.loadTimeData = {
  getString,
  getBoolean(key: string) {
    return false
  },
  getInteger(key: string) {
    return 0
  },
}

if (!global.chrome) global.chrome = { extension: {} }
global.chrome.extension = {
  inIncognitoContext: false,
}

export default {
  decorators: [
    // Mirror production: restore styled-components v5 DOM prop filtering so
    // custom style-only props don't leak onto DOM nodes as invalid attributes.
    (Story: () => JSX.Element) => (
      <StyledComponentsProvider>
        <Story />
      </StyledComponentsProvider>
    ),
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
    withKnobs,
  ],
}
