/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import * as ReactDOM from 'react-dom'

document.addEventListener('DOMContentLoaded', () => {
  console.error('Hello from JS!');

  setInterval(() => {
    console.error('tick from JS')
  }, 1000)

  document.body.addEventListener('keyup', (event) => {
    if (event.key.toLowerCase() === 'escape') {
      chrome.send('ClosePanel');
      // Close the panel?
    }
  })

  function Root () {
    return (
      <>Hello world</>
    )
  }

  ReactDOM.render(<Root />, document.getElementById('root'))

  chrome.send('PageReady');
})
