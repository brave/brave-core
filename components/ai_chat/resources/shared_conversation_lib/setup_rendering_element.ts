// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { addStyleTarget } from './style_loader'

async function createNalaStyleElement(shadow: ShadowRoot) {
  const style = document.createElement('style')
  const scriptUrl = import.meta.url
  const relativePathUrl = new URL('./nala.css', scriptUrl)
  style.textContent = await fetch(relativePathUrl).then((res) => res.text())
  shadow.appendChild(style)
}

/**
 * Sets up a rendering element for the shared conversation viewer.
 *
 * @param element - the DOM element to render the conversation into
 * @returns the shadow root or container element for rendering
 */
export default function setupRenderingElement(element: HTMLElement) {
  const shadow = element.attachShadow({ mode: 'open' })
  // Add nala styles only to the shadow
  createNalaStyleElement(shadow)
  // Add conversation rendering styles only to the shadow
  addStyleTarget(shadow)
  // Create a container for react to render in
  const container = document.createElement('div')
  container.style.display = 'contents'
  shadow.appendChild(container)

  return container
}
