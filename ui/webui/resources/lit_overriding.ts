// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { CrLitElement, CSSResultGroup } from "//resources/lit/v3_0/lit.rollup.js";

export function injectStyle<T extends typeof CrLitElement>(
  element: T,
  css: CSSResultGroup)
{
  const originalStyles = element.styles
  Object.defineProperty(element, 'styles', {
    get() {
      return [
        ...originalStyles as any,
        css
      ]
    }
  })

  element.elementStyles = (element as any).finalizeStyles(element.styles)
}
