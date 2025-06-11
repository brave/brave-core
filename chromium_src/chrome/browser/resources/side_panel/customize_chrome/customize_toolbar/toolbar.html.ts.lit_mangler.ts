// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

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
