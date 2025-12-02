// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// -----------------------------------------------------------------------------
// Scoped CSS for Front-end Components
// -----------------------------------------------------------------------------
//
// Overview:
//
// This module provides CSS scoping using the native CSS `@scope` at-rule.
// Styles defined with `scoped.css` are automatically scoped to the component
// and won't leak into child components that define their own scope.
//
// Basic usage:
//
// 1. Create a style file for your component:
//
//   // my_component.style.ts
//   import { scoped } from '$web-common/scoped_css'
//
//   export const style = scoped.css`
//     & {
//       display: flex;
//       padding: 16px;
//     }
//
//     .title {
//       font-weight: bold;
//     }
//   `
//
// 2. Apply the scope to your front-end component:
//
//   // my_component.tsx
//   import { style } from './my_component.style'
//
//   export function MyComponent() {
//     return (
//       <div data-css-scope={style.scope}>
//         <h1 className="title">Hello</h1>
//       </div>
//     )
//   }
//
// Passthrough styles:
//
// Use `style.passthrough.css` when you need styles to apply to nested scoped
// components (e.g., for flex container properties):
//
//   style.passthrough.css`
//     .child-container {
//       flex: 1;
//     }
//   `

const stylesheetMap = new Map<string, CSSStyleSheet>()

// Adds CSS to the document. If a stylesheet with the specified `id` has already
// been added to the document, then it will be replaced with the provided CSS.
async function addStyles(id: string, cssText: unknown) {
  let stylesheet = stylesheetMap.get(id)
  if (!stylesheet) {
    stylesheet = new CSSStyleSheet()
    stylesheetMap.set(id, stylesheet)
    document.adoptedStyleSheets.push(stylesheet)
  }
  await stylesheet.replace(String(cssText))
}

const scopeAttributeName = 'data-css-scope'

class ScopedCSSAttribute {
  [scopeAttributeName]: string

  constructor(scopeName: string) {
    this[scopeAttributeName] = scopeName
  }

  get scope() {
    return this[scopeAttributeName]
  }

  get selector() {
    return `[${scopeAttributeName}=${CSS.escape(this[scopeAttributeName])}]`
  }

  get passthrough() {
    const { selector } = this
    return {
      // Adds styles that "pass-through" descendants with a "data-css-scope"
      // attribute. Pass-through styles can be useful for containers that need
      // to supply styling to children (e.g. flex properties) that might
      // themselves define a style scope.
      css(callsite: TemplateStringsArray, ...values: any[]) {
        addStyles(
          `${selector}-passthrough`,
          `@scope (${selector}) { ${String.raw(callsite, ...values)} }`
        )
      },
    }
  }
}

let nextScopeID = 0x5c09ed

// A template tag that adds scoped CSS to the document. The provided CSS text
// is wrapped with a "@scope" at-rule and only applies to elements with a
// "data-css-scope" attribute whose value matches `scopeName`. The CSS rules do
// not apply to any descendant elements that have a "data-css-scope" attribute.
// Returns an object representing the CSS scope data attribute, which can be
// object-spread into a collection of HTML attributes.
export const scoped = {
  css(callsite: TemplateStringsArray, ...values: any[]) {
    const id = (nextScopeID++).toString(36)
    const attr = new ScopedCSSAttribute(id)
    addStyles(
      attr.selector,
      `@scope (${attr.selector}) to ([${scopeAttributeName}]) {
        ${String.raw(callsite, ...values)}
      }`
    )
    return attr
  },
}
