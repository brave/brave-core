/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

const stylesheetMap = new Map<string, CSSStyleSheet>()

export const css = String.raw

// Adds CSS to the document. If a stylesheet with the specified `id` has already
// been added to the document, then it will be replaced with the provided CSS.
export async function addStyles(id: string, cssText: unknown) {
  if (!id) {
    throw new Error('Argument "id" cannot be empty')
  }

  let stylesheet = stylesheetMap.get(id)
  if (!stylesheet) {
    stylesheet = new CSSStyleSheet()
    stylesheetMap.set(id, stylesheet)
    document.adoptedStyleSheets.push(stylesheet)
  }

  await stylesheet.replace(String(cssText))
}

