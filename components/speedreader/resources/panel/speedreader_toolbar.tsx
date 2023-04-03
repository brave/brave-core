// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from 'react-dom'
import { initLocale } from 'brave-ui'
import { ToolbarWrapper } from './style'

import { loadTimeData } from '../../../common/loadTimeData'
import BraveCoreThemeProvider from '../../../common/BraveCoreThemeProvider'
import Container from './container'
import { addWebUiListener } from 'chrome://resources/js/cr.js';
import { $ } from 'chrome://resources/js/util_ts.js';

function App () {
  return (
    <BraveCoreThemeProvider>
      <ToolbarWrapper>
        <Container />
      </ToolbarWrapper>
    </BraveCoreThemeProvider>
  )
}

function initialize () {
  addWebUiListener('theme-changed', () => {
    ($('colors') as HTMLLinkElement).href = 'chrome://theme/colors.css?sets=ui,chrome&version=' + Date.now();
  });
  chrome.send('observeThemeChanges');

  initLocale(loadTimeData.data_)
  render(<App />, document.getElementById('mountPoint'))
}

document.addEventListener('DOMContentLoaded', initialize)
