/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import * as ReactDOM from 'react-dom'

import { App } from './components/app'
import { createHost } from './lib/host'
import { HostContext } from './lib/host_context'
import { LocaleContext } from '../shared/lib/locale_context'

const host = createHost()

document.addEventListener('DOMContentLoaded', () => {
  document.body.addEventListener('keyup', (event) => {
    if (event.key.toLowerCase() === 'escape') {
      host.closeDialog()
    }
  })

  function Root () {
    return (
      <HostContext.Provider value={host}>
        <LocaleContext.Provider value={host}>
          <App />
        </LocaleContext.Provider>
      </HostContext.Provider>
    )
  }

  ReactDOM.render(<Root />, document.getElementById('root'))
})
