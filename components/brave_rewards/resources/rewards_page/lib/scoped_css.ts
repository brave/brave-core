/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { addStyles } from './style_injector'

export const css = String.raw

const scopeAttributeName = 'data-css-scope'

class ScopedCSSAttribute {
  [scopeAttributeName]: string

  constructor(scopeName: string) {
    this[scopeAttributeName] = scopeName
  }

  get selector() {
    return `[${scopeAttributeName}=${CSS.escape(this[scopeAttributeName])}]`
  }
}

// Adds scoped CSS to the document. The provided `cssText` is wrapped with a
// "@scope" at-rule and only applies to elements with a "data-css-scope"
// attribute whose value matches `scopeName`. The CSS rules do not apply to any
// descendant elements that have a "data-css-scope" attribute. The scope name
// should be globally unique. Returns an object representing the CSS scope data
// attribute, which can be object-spread into a collection of HTML attributes.
export function scopedCSS(scopeName: string, cssText: unknown) {
  const attr = new ScopedCSSAttribute(scopeName)
  addStyles(attr.selector, css`
    @scope (${attr.selector}) to ([${scopeAttributeName}]) {
      ${cssText}
    }
  `)
  return attr
}
