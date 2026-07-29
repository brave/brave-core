// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * Receives the style elements that style-loader creates for our bundled CSS and
 * copies them in to shadow roots instead of the embedding page's document.
 *
 * webpack.config.ts wires the default export up as style-loader's `insert`
 * option - see the comment there for how that connection is made.
 */

// Every style element style-loader has given us, in insertion order.
const styleElements: HTMLStyleElement[] = []

// Each target we copy styles to, with the number of `styleElements` it has
// received so far. A cursor rather than a flag, so that CSS which arrives after
// a target is registered - from a lazily-loaded chunk, e.g. code_block - still
// reaches it.
const targets = new Map<ParentNode, number>()

function copyPendingStyles() {
  for (const [target, copiedCount] of targets) {
    for (const element of styleElements.slice(copiedCount)) {
      target.appendChild(element.cloneNode(true))
    }
    targets.set(target, styleElements.length)
  }
}

/**
 * Renders all our bundled CSS, including CSS from chunks which load later, in
 * to `target` (e.g. a ShadowRoot). Copies are made, so more than one target can
 * be registered.
 */
export function addStyleTarget(target: ParentNode) {
  targets.set(target, 0)
  copyPendingStyles()
}

/**
 * Called by style-loader for each style element it creates. The elements
 * themselves are never attached to the document, so nothing we bundle can style
 * - or be styled by - the page hosting this library.
 */
export default function insertStyleElement(element: HTMLStyleElement) {
  styleElements.push(element)
  // style-loader sets the element's CSS text immediately after this returns, so
  // let the current task finish before copying it anywhere.
  queueMicrotask(copyPendingStyles)
}
