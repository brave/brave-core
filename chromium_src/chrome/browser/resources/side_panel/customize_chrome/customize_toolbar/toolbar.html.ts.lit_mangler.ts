// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

// Insert icon to "Toolbar" section's heading
mangle(
  (element: DocumentFragment) => {
    const firstSpCard = element.querySelector('.sp-card')
    if (!firstSpCard) {
      throw new Error('[Customize Chrome > Toolbar] .sp-card is gone.')
    }

    const headingEl = firstSpCard.querySelector('sp-heading h2[slot="heading"]')
    if (!headingEl) {
      throw new Error(
        '[Customize Chrome > Toolbar] <sp-heading h2[slot="heading"]> is gone.',
      )
    }

    headingEl.insertAdjacentHTML(
      'afterbegin',
      /* html */ `<leo-icon name="window-edit"></leo-icon>`,
    )
  },
  /* omit selector to access the top level node */
)

// Remove #miniToolbarBackground
mangle(
  (element: DocumentFragment) => {
    // Remove the mini toolbar background.
    const miniToolbarBackground = element.querySelector(
      '#miniToolbarBackground',
    )
    if (!miniToolbarBackground) {
      throw new Error(
        '[Customize Chrome > Toolbar] #miniToolbarBackground is gone.',
      )
    }

    miniToolbarBackground.remove()
  },
  (template) => template.text.includes('id="miniToolbarBackground"'),
)

// Remove #tipCard
mangle(
  (element: DocumentFragment) => {
    // Remove the tip card.
    const tipCard = element.querySelector('#tipCard')
    if (!tipCard) {
      throw new Error('[Customize Chrome > Toolbar] #tipCard is gone.')
    }

    tipCard.remove()
  },
  (template) => template.text.includes('id="tipCard"'),
)

// Move .intro-text out of the sp-card and insert it after the sp-card, wrapping
// it with a new sp-card. The first sp-card is the one with the heading.
mangle(
  (element: DocumentFragment) => {
    const introText = element.querySelector('.intro-text')
    if (!introText) {
      throw new Error('[Customize Chrome > Toolbar] .intro-text is gone.')
    }

    // Remove the intro text from its current position.
    const firstSpCard = element.querySelector('.sp-card')
    if (!firstSpCard) {
      throw new Error('[Customize Chrome > Toolbar] .sp-card is gone.')
    }
    firstSpCard.removeChild(introText)

    // As we are inserting the bundled HTML at build time, we don't have to sanitize the element.
    // eslint-disable-next-line no-unsanitized/method
    firstSpCard.insertAdjacentHTML(
      'afterend',
      /* html */ `
      <div class="sp-card">
        ${introText.outerHTML}
      </div>
    `,
    )
  },
  /* omit selector to access the top level node */
)

// Insert tipCardLabel text after ".intro-text"
mangle(
  (element: DocumentFragment) => {
    const introText = element.querySelector('.intro-text')
    if (!introText) {
      throw new Error('[Customize Chrome > Toolbar] .intro-text is gone.')
    }

    introText.insertAdjacentHTML(
      'afterend',
      /* html */ `
        <div id="tipCardLabel">$i18n{reorderTipLabel}</div>
    `,
    )
  },
  (template) => template.text.includes('class="intro-text"'),
)

// Move #resetToDefaultButton to the end
mangle(
  (element: DocumentFragment) => {
    // Remove the reset button.
    const resetButton = element.querySelector('#resetToDefaultButton')
    if (!resetButton) {
      throw new Error(
        '[Customize Chrome > Toolbar] #resetToDefaultButton is gone.',
      )
    }

    resetButton.remove()

    const lastEl = element.querySelector('#pinningSelectionCard')
    if (!lastEl) {
      throw new Error(
        '[Customize Chrome > Toolbar] #pinningSelectionCard is gone.',
      )
    }
    lastEl.insertAdjacentElement('afterend', resetButton)
  } /* omit selector to access the top level node */,
)

// Insert close button to the sp-heading element
mangle(
  (element: DocumentFragment) => {
    const el = element.querySelector('sp-heading')
    if (!el) {
      throw new Error('[Customize Chrome > Toolbar] sp-heading is gone.')
    }

    el.insertAdjacentHTML(
      'afterbegin',
      /* html */ `
        <close-panel-button id="closeButton" iron-icon="close" slot="buttons" />`,
    )
  },
  (template) => template.text.includes('sp-heading'),
)

// Hide back button
mangle(
  (element: DocumentFragment) => {
    const el = element.querySelector('sp-heading')
    if (!el) {
      throw new Error('[Customize Chrome > Toolbar] sp-heading is gone.')
    }

    el.setAttribute('hide-back-button', 'true')
  },
  (template) => template.text.includes('sp-heading'),
)
