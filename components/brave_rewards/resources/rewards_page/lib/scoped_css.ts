/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { addStyles, css } from './style_injector'

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

let scopeID = 0xa

export const scoped = {
  // Adds scoped CSS to the document. The provided `cssText` is wrapped with a
  // "@scope" at-rule and only applies to elements with a "data-css-scope"
  // attribute whose value matches `scopeName`. The CSS rules do not apply to
  // any descendant elements that have a "data-css-scope" attribute. The scope
  // name should be globally unique. Returns an object representing the CSS
  // scope data attribute, which can be object-spread into a collection of HTML
  // attributes.
  css(callsite: TemplateStringsArray, ...values: any[]) {
    const attr = new ScopedCSSAttribute((scopeID++).toString(36))
    addStyles(attr.selector, css`
      @scope (${attr.selector}) to ([${scopeAttributeName}]) {
        ${css(callsite, ...values)}
      }
    `)
    return attr
  }
}
