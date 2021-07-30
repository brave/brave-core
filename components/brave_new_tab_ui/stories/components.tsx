// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { withKnobs } from '@storybook/addon-knobs'
import ThemeProvider from '../../common/StorybookThemeProvider'
// Components
import LoadingComponent from '../components/loading'
import OutlineButtonComponent from '../components/outlineButton'

export default {
  title: 'New Tab',
  decorators: [
    (Story: any) => <ThemeProvider><Story /></ThemeProvider>,
    withKnobs
  ]
}

export const Loading = () => (
  <div
    style={{ width: '500px', height: '500px', display: 'flex', alignItems: 'center', justifyContent: 'center' }}
  >
    <LoadingComponent />
  </div>
)

export const OutlineButton = () => (
  <div
    style={{ width: '500px', height: '500px', display: 'flex', alignItems: 'center', justifyContent: 'center' }}
  >
    <OutlineButtonComponent>
      Button
    </OutlineButtonComponent>
  </div>
)
