/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

export const cssTextSymbol = Symbol('cssText')

let pendingCSS = ''

function inject() {
  if (!pendingCSS) {
    return
  }

  const text = pendingCSS
  pendingCSS = ''

  const element = document.createElement('style')
  element.append(text)

  const root = document.documentElement
  const parent = document.head || root.firstElementChild || root
  parent.append(element)
}

// Adds CSS to the document.
export function addStyles(cssText: any) {
  const scheduled = pendingCSS !== ''

  pendingCSS += String(cssText)

  if (!scheduled) {
    if (document.readyState === 'loading') {
      document.addEventListener('DOMContentLoaded', inject)
    } else {
      Promise.resolve().then(inject)
    }
  }
}

class CssTemplateResult {
  [cssTextSymbol]: string
  constructor(cssText: string) { this[cssTextSymbol] = cssText }
  toString() { return this[cssTextSymbol] }
}

// A template tag for building CSS text.
export function css(callsite: TemplateStringsArray, ...values: any[]) {
  let cssText = ''
  let i = 0
  for (const part of callsite.raw) {
    cssText += part
    const value = values[i++]
    if (value !== null && value !== undefined) {
      cssText += value[cssTextSymbol] || String(value)
    }
  }
  return new CssTemplateResult(cssText)
}

