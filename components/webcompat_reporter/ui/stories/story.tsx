/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { storiesOf } from '@storybook/react'
import Theme from 'brave-ui/theme/brave-default'
import DarkTheme from 'brave-ui/theme/brave-dark'
import { withThemesProvider } from 'storybook-addon-styled-component-theme'
// @ts-ignore
import { boolean, number } from '@storybook/addon-knobs'

// Components
import WebcompatReportModal from './modal/index'

// Themes
const themes = [Theme, DarkTheme]

storiesOf('Webcompat Reporter', module)
  .addDecorator(withThemesProvider(themes))
  .add('Before submit', () => {
    const fakeOnSubmit = () => { /* noop */ }
    const fakeOnClose = () => { /* noop */ }
    return (
      <div style={{ width: '375px', margin: '0 auto' }}>
        <WebcompatReportModal
          siteUrl={'https://www.buzzfeed.com/'}
          submitted={false}
          onSubmitReport={fakeOnSubmit}
          onClose={fakeOnClose}
        />
      </div>
    )
  })
  .add('Submit confirmation', () => {
    const fakeOnSubmit = () => { /* noop */ }
    const fakeOnClose = () => { /* noop */ }
    return (
      <div style={{ width: '375px', margin: '0 auto' }}>
        <WebcompatReportModal
          siteUrl={'https://www.buzzfeed.com/'}
          submitted={true}
          onSubmitReport={fakeOnSubmit}
          onClose={fakeOnClose}
        />
      </div>
    )
  })
