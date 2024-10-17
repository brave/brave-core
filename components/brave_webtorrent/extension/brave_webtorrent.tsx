// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import { Provider } from 'react-redux'
import { Store } from 'webext-redux'

import Theme from 'brave-ui/theme/brave-default'
import { createGlobalStyle, ThemeProvider } from 'styled-components'

// Components
import App from './components/app'

// Constants
import { ApplicationState } from './constants/webtorrentState'

import '$web-components/app.global.scss'

// This is a hack that's needed for lazy loading
// Basically we need the browser to restart the navigation, so we redirect first
// to brave_webtorrent2.html and have that rewrite the URL to
// brave_webtorrent.html
if (window.location.pathname === '/extension/brave_webtorrent2.html') {
  window.location.href = window.location.href.replace(
    'brave_webtorrent2.html',
    'brave_webtorrent.html'
  )
}

const GlobalStyle = createGlobalStyle`
  body {
    background: var(--background2);
  }
`

const store: Store<ApplicationState> = new Store({
  portName: 'WEBTORRENT'
})

// This pattern is described in the webext-redux docs. When the proxy store is
// ready, it doesn't necessarily have a value (and might be {}). If we want
// the same store value as the background page, we should wait for the first
// update and unsubscribe.
// https://github.com/tshaddix/webext-redux/wiki/Advanced-Usage#initializing-ui-components
const unsubscribe = store.subscribe(async () => {
  unsubscribe()

  try {
    const tab: any = await new Promise((resolve) =>
      chrome.tabs.getCurrent(resolve)
    )

    const container = document.getElementById('root')
    const root = createRoot(container!)

    root.render(
      <Provider store={store}>
        <GlobalStyle />
        <ThemeProvider theme={Theme}>
          <App tabId={tab.id} />
        </ThemeProvider>
      </Provider>
    )
  } catch (err) {
    console.log('Problem mounting webtorrent', err)
  }
})
