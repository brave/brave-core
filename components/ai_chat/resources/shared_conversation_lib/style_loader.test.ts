// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { describe, it, expect } from '@jest/globals'
import insertStyleElement, { addStyleTarget } from './style_loader'

/**
 * Mimics what style-loader does for each stylesheet in the bundle: it hands us a
 * brand new style element and only fills in the CSS text afterwards.
 */
function insertStyleElementWithCss(css: string) {
  const element = document.createElement('style')
  insertStyleElement(element)
  element.appendChild(document.createTextNode(css))
}

function createShadowRoot() {
  return document.createElement('div').attachShadow({ mode: 'open' })
}

function cssIn(target: ParentNode) {
  return [...target.querySelectorAll('style')].map((style) => style.textContent)
}

describe('shared conversation style loader', () => {
  // Module state is shared between the steps below, mirroring how a page loads
  // the library once and can then render more than one conversation.
  it('sends bundled CSS to every target, including CSS arriving later', async () => {
    // CSS bundled with the entrypoint arrives before anything is rendered.
    insertStyleElementWithCss('.a {}')

    const first = createShadowRoot()
    addStyleTarget(first)
    expect(cssIn(first)).toEqual(['.a {}'])

    // CSS from a chunk which loads after the conversation was rendered, e.g.
    // the lazily imported code block component. Copying is deferred to a
    // microtask because the CSS text isn't set yet.
    insertStyleElementWithCss('.b {}')
    expect(cssIn(first)).toEqual(['.a {}'])
    await Promise.resolve()
    expect(cssIn(first)).toEqual(['.a {}', '.b {}'])

    // A second conversation rendered on the same page gets its own copies, and
    // the first target isn't sent anything twice.
    const second = createShadowRoot()
    addStyleTarget(second)
    expect(cssIn(second)).toEqual(['.a {}', '.b {}'])
    expect(cssIn(first)).toEqual(['.a {}', '.b {}'])
  })
})
