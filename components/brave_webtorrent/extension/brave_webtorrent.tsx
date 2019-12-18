/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { render } from 'react-dom'
import { Provider } from 'react-redux'
import { Store } from 'webext-redux'

import Theme from 'brave-ui/theme/brave-default'
import { ThemeProvider } from 'brave-ui/theme'

// Components
import App from './components/app'

// Constants
import { TorrentsState } from './constants/webtorrentState'

// This is a hack that's needed for lazy loading
// Basically we need the browser to restart the navigation, so we redirect first
// to brave_webtorrent2.html and have that rewrite the URL to
// brave_webtorrent.html
if (window.location.pathname === '/extension/brave_webtorrent2.html') {
  window.location.href =
    window.location.href.replace('brave_webtorrent2.html', 'brave_webtorrent.html')
}

const store: Store<TorrentsState> = new Store({
  portName: 'WEBTORRENT'
})

store.ready().then(
  async () => {
    const tab: any = await new Promise(resolve => chrome.tabs.getCurrent(resolve))
    render(
      <Provider store={store}>
        <ThemeProvider theme={Theme}>
          <App tabId={tab.id} />
        </ThemeProvider>
      </Provider>,
      document.getElementById('root'))
  })
  .catch(() => {
    console.error('Problem mounting webtorrent')
  })
