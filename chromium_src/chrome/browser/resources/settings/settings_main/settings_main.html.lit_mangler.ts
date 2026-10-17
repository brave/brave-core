// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

const getById = (root: DocumentFragment, id: string) => {
  const element = root.getElementById(id)
  if (!element) {
    throw new Error(`[Settings] Main: couldn't find #${id}`)
  }
  return element
}

// A view shown according to `pageVisibility`, with the same markup as
// upstream's views.
const view = (
  id: string,
  visibilityKey: string,
  route: string,
  element: string,
) =>
  `\${cache(this.showPage_(this.pageVisibility_?.${visibilityKey}) ? html\`
    <div slot="view" id="${id}">
      \${cache(this.renderPlugin_(this.routes_.${route}) ? html\`
        <${element} ?in-search-mode="\${this.inSearchMode_}"></${element}>
      \` : '')}
    </div>
  \` : '')}`

// Performance lives under System instead.
mangle(
  (root) => getById(root, 'performance').remove(),
  (t) => t.text.includes('id="performance"'),
)

mangle((root) => {
  const switcher = getById(root, 'switcher')
  // Web3 and Leo are hidden by `pageVisibility` when disallowed.
  switcher.insertAdjacentHTML('beforeend',
    view('getStarted', 'getStarted', 'GET_STARTED',
         'settings-getting-started-page-index')
    + view('origin', 'origin', 'ORIGIN', 'settings-brave-origin-page')
    + view('content', 'content', 'BRAVE_CONTENT',
           'settings-brave-content-page-index')
    + view('shields', 'shields', 'SHIELDS', 'settings-shields-page-index')
    + view('web3', 'braveWallet', 'BRAVE_WEB3', 'settings-wallet-page-index')
    + view('leoAssistant', 'leoAssistant', 'BRAVE_LEO_ASSISTANT',
           'settings-brave-leo-assistant-page-index')
    + view('braveSync', 'braveSync', 'BRAVE_SYNC',
           'settings-brave-sync-page-index'))

  // Extensions has no route of its own to wait for, so it always renders.
  switcher.insertAdjacentHTML('beforeend',
    `\${cache(this.showPage_(this.pageVisibility_?.extensions) ? html\`
      <div slot="view" id="extensions">
        <settings-brave-extensions-page-index
            ?in-search-mode="\${this.inSearchMode_}">
        </settings-brave-extensions-page-index>
      </div>
    \` : '')}`)
}, (t) => t.text.includes('id="switcher"'))

// Upstream doesn't have sub pages for System.
mangle((root) => {
  const systemPage = root.querySelector('settings-system-page')
  if (!systemPage) {
    throw new Error(`[Settings] Main: couldn't find settings-system-page`)
  }
  systemPage.replaceWith(
    root.ownerDocument.createElement('settings-brave-system-page-index'))
}, (t) => t.text.includes('<settings-system-page>'))
