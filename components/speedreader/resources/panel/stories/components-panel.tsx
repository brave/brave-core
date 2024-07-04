// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import * as S from './style'
import { withKnobs } from '@storybook/addon-knobs'

import './locale'
import Toolbar from '../components/toolbar'
import ThemeProvider from '$web-common/BraveCoreThemeProvider'

export default {
  title: 'Speedreader/Toolbar',
  parameters: {
    layout: 'centered'
  },
  argTypes: {
    locked: { control: { type: 'boolean', lock: false } }
  },
  decorators: [
    (Story: any) => {
      return (
        <ThemeProvider>
          <Story />
        </ThemeProvider>
      )
    },
    withKnobs
  ],
  component: Toolbar
}

export const Main = {
  render: () => <S.PanelFrame>
    <Toolbar />
  </S.PanelFrame>
}
