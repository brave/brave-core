/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import * as ReactDOM from 'react-dom'

import { App } from './components/app'
import { createHost } from './lib/extension_host'
import { LocaleContext } from '../shared/lib/locale_context'
import { createLocaleContextForWebUI } from '../shared/lib/webui_locale_context'

function onReady () {
  ReactDOM.render(
    <LocaleContext.Provider value={createLocaleContextForWebUI()}>
      <App host={createHost()} />
    </LocaleContext.Provider>,
    document.getElementById('root'))
}

if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', onReady)
} else {
  onReady()
}
