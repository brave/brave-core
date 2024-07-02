/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { addStyles, css } from './style_injector'

export * from './style_injector'

let componentCount = 0

function uniqueID() {
  let id = String(componentCount).padStart(4, '0')
  componentCount = (componentCount + 1) >> 0
  return `cs-${id}`
}

type Props =
    React.ClassAttributes<HTMLElement> & React.HTMLAttributes<HTMLElement>

// Returns a component that renders a `div` with the "component-style" CSS class
// name. The provided CSS rules are scoped (using the @scope "at-rule") to only
// the rendered element. The scope does not apply to any nested elements that
// have the "component-style" CSS class name.
export function componentStyle(className: string, scopedCSS: any) {
  // Create a unique ID so that if style components are regenerated (e.g. within
  // a storybook dev session) the old styles will no longer be applied.
  const id = uniqueID()
  addStyles(css`
    @scope (.${CSS.escape(className)}.${id}) to (.component-style) {
      ${scopedCSS}
    }
  `)
  return (props: Props) => React.createElement('div', {
    ...props,
    className: `${className} ${id} component-style`
  })
}
