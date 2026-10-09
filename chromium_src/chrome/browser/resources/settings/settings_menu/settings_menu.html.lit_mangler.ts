// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

const getById = (root: DocumentFragment, id: string) => {
  const element = root.getElementById(id)
  if (!element) {
    throw new Error(`[Settings] Menu: couldn't find #${id}`)
  }
  return element
}

// Brave's menu items are shown according to `pageVisibility`, with the same
// markup as upstream's items. `id` doubles as the page visibility key.
const menuItem = (
  id: string,
  href: string,
  icon: string,
  title: string,
  visibilityKey: string = id,
) =>
  `<a role="menuitem" id="${id}" href="${href}" class="cr-nav-menu-item"
      ?hidden="\${this.shouldHideMenuItem_(
          this.pageVisibility_?.${visibilityKey})}">
    <cr-icon icon="${icon}"></cr-icon>
    <span class="menu-label">$i18n{${title}}</span>
    <cr-ripple></cr-ripple>
  </a>`

// Mangling happens at build time, over the template shipped with the browser,
// so there is no untrusted input to sanitize here.
const insertAfter = (element: Element, html: string) =>
  // eslint-disable-next-line no-unsanitized/method
  element.insertAdjacentHTML('afterend', html)

mangle((root) => {
  // Performance lives under System instead.
  getById(root, 'performance').remove()

  // Get Started and Origin follow People, then Appearance, Content, Shields and
  // Privacy.
  insertAfter(getById(root, 'people'),
    menuItem('getStarted', '/getStarted', 'rocket', 'braveGetStartedTitle')
    + menuItem('origin', '/origin', 'product-origin', 'braveOriginTitle'))

  const appearance = getById(root, 'appearance')
  getById(root, 'origin').after(appearance)
  insertAfter(appearance,
    menuItem('content', '/braveContent', 'window-content',
             'contentSettingsContentSection')
    + menuItem('shields', '/shields', 'shield-done', 'braveShieldsTitle'))

  const shields = getById(root, 'shields')
  shields.after(getById(root, 'privacy'))

  // Web3 and Leo are hidden by `pageVisibility` when disallowed. The Leo string
  // isn't registered without AI Chat, so it is only rendered when visible.
  insertAfter(getById(root, 'privacy'),
    menuItem('braveWallet', '/web3', 'product-brave-wallet', 'braveWeb3')
    + `\${this.pageVisibility_?.leoAssistant ? html\`
      ${menuItem('leoAssistant', '/leo-ai', 'product-brave-leo',
                 'leoAssistant')}
    \` : ''}`
    + menuItem('braveSync', '/braveSync', 'product-sync', 'braveSync'))

  // Search and Extensions follow Sync.
  getById(root, 'braveSync').after(getById(root, 'search'))
  insertAfter(getById(root, 'search'),
    menuItem('extensions', '/extensions', 'browser-extensions',
             'braveDefaultExtensions'))

  // Autofill is an advanced setting.
  getById(root, 'languages').before(getById(root, 'autofill'))

  getById(root, 'extensionsLink').remove()

  // Show the version under the About link.
  const about = getById(root, 'about-menu')
  about.insertAdjacentHTML('afterend',
    `<a role="menuitem" id="about-menu" href="/help">
      <div class="brave-about-graphic">
        <img srcset="chrome://theme/current-channel-logo@1x,
                     chrome://theme/current-channel-logo@2x 2x"
            width="20px" height="20px">
      </div>
      <div class="brave-about-meta">
        <span class="brave-about-item brave-about-menu-link-text">
          $i18n{aboutPageTitle}</span>
        <span class="brave-about-item brave-about-menu-version">
          v $i18n{braveProductVersion}</span>
      </div>
    </a>`)
  about.remove()
})
