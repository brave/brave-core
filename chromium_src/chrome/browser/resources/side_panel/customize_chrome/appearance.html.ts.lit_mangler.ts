// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

function findElementWithId(
  elementId: string,
  element: DocumentFragment,
): Element {
  const el = element.querySelector(`#${elementId}`)
  if (!el) {
    throw new Error(`[Customize Chrome > Appearance] #${elementId} is gone.`)
  }

  return el
}

function hideElementWithId(elementId: string) {
  mangle(
    (element: DocumentFragment) => {
      findElementWithId(elementId, element).style.display = 'none'
    },
    (template) => template.text.includes(`id="${elementId}"`),
  )
}

// Hides preview image.
hideElementWithId('themeSnapshot')

// Hides the label and button for "Brave is managing your new tab page"
hideElementWithId('thirdPartyManageLinkButton')

// Moves the edit buttons container to the end of the template.
mangle(
  (element: DocumentFragment) => {
    // Move the element to the end.
    const el = findElementWithId('editButtonsContainer', element)

    // Note that we use parentNode instead of parent element as there's no
    // parent container for this element.
    if (!el.parentNode) {
      throw new Error('No parent node')
    }

    el.parentNode.appendChild(el)
  },
  (template) => template.text.includes('id="editButtonsContainer"'),
)
